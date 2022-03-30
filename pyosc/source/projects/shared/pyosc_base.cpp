#include "pyosc_base.h"


BaseOscObject::BaseOscObject(const std::string& name
                             , const std::string& status_address)
        : name(name)
          , address("/" + name)
          , status_address(status_address)
          , status(Status::offline) {
    bool res = PyOscManager::get_instance().add_object(shared_from_this());
    if (!res) {
        throw std::runtime_error("an object with the name " + name + " already exists");
    }
}

BaseOscObject::~BaseOscObject() {
    delete_object();
}

Status BaseOscObject::parse_status(osc::ReceivedMessage& msg) {
    if (msg.ArgumentCount() != 1) {
        return Status::invalid_status;
    }

    int status_code;
    auto arg = msg.ArgumentsBegin();

    if (arg->IsInt32()) {
        status_code = static_cast<int>(arg->AsInt32());
    } else if (arg->IsInt64()) {
        status_code = static_cast<int>(arg->AsInt64());
    } else {
        return Status::invalid_status;
    }

    if (status_code >= static_cast<int>(MIN_STATUS) && status_code <= static_cast<int>(MAX_STATUS)) {
        return static_cast<Status>(status_code);
    }

    return Status::invalid_status;
}

Status BaseOscObject::read_server_status() {
    if (!initialized) {
        status = Status::uninitialized;
    } else {
        // initialize has been called but has not yet received any response from the server
        if (!last_response) {
            status = Status::initializing;
        }

        // Read status messages from receiver
        auto new_status_values = receive(status_address);

        // If status message received: set status to received message and update time
        if (!new_status_values.empty()) {
            auto last_status_candidate = new_status_values.back();
            status = parse_status(last_status_candidate);
            last_response = std::chrono::system_clock::now();

        } else if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() -
                                                                    *last_response).count() > TIMEOUT_SECONDS) {
            status = Status::no_response;
        }
    }
    return status;
}

Connector BaseOscObject::create_connector(const std::string& ip, std::optional<PortSpec> port_spec) {
    PyOscManager::get_instance().new_connector(ip, port_spec);
}

std::shared_ptr<BaseOscObject> BaseOscObject::get_object(std::string& object_name) {
    // TODO
}


void BaseOscObject::delete_object() {
    terminate();
    status = Status::deleted;
    PyOscManager::get_instance().remove_object(name);
}


const std::string& BaseOscObject::get_address() const {
    return address;
}

void BaseOscObject::reallocate_connector_ports(Connector& connector) {
    PyOscManager::get_instance().reallocate_connector(connector);
}


//  ================================================================================================= //
//  == PyOscManager ==
//  ================================================================================================= //


PyOscManager& PyOscManager::get_instance() {
    static PyOscManager instance; // Guaranteed to be destroyed.
    return instance; // Instantiated on first use.
}


bool PyOscManager::add_object(std::shared_ptr<BaseOscObject> object) {
    std::lock_guard<std::mutex> lock{mutex};
    if (name_exists(object->get_name())) {
        return false;
    }

    objects.emplace_back(object);
    return true;
}


std::shared_ptr<BaseOscObject> PyOscManager::get_object(const std::string& name) {
    std::lock_guard<std::mutex> lock{mutex};

    for (auto& obj: objects) {
        if (obj->get_name() == name) {
            return obj;
        }
    }
    return nullptr;
}


void PyOscManager::remove_object(const std::string& name) {
    std::lock_guard<std::mutex> lock{mutex};

    objects.erase(std::remove_if(objects.begin(), objects.end(), [&name](auto obj) {
        return obj->get_name() == name;
    }), objects.end());
}


Connector PyOscManager::new_connector(const std::string& ip, std::optional<PortSpec> port_spec) {
    std::lock_guard<std::mutex> lock{mutex};


    int send_port = -1;
    int recv_port = -1;

    // Manual port assignment
    if (port_spec) {
        send_port = port_spec->send_port;
        recv_port = port_spec->recv_port;

        if (!port_is_bound(send_port) && port_is_available(send_port)
            && !port_is_bound(recv_port) && port_is_available(recv_port)) {
            block_port(send_port);
            block_port(recv_port);
            return {ip, send_port, recv_port};

        } else {
            throw std::runtime_error("port is not available");
        }

        // Automatic port assignment
    } else {
        auto port_it = available_ports.begin();

        while (send_port == -1 && port_it != available_ports.end()) {
            if (!port_is_bound(*port_it)) {
                send_port = *port_it;
                port_it = available_ports.erase(port_it);
            } else {
                ++port_it;
            }
        }

        if (send_port == -1) {
            throw std::runtime_error("not enough ports available");
        }

        // Continue on same iterator
        while (recv_port == -1 && port_it != available_ports.end()) {
            if (!port_is_bound(*port_it)) {
                recv_port = *port_it;
                port_it = available_ports.erase(port_it);
            } else {
                ++port_it;
            }
        }

        if (recv_port == -1) {
            // deallocate send_port since this was already assigned
            unblock_port(send_port);
            throw std::runtime_error("not enough ports available");
        }

        return {ip, send_port, recv_port};

    }

}

bool PyOscManager::reallocate_connector(Connector& connector) {
    unblock_port(connector.get_send_port());
    unblock_port(connector.get_recv_port());
}


// ===============================================================
// PyOscManager::private
// ===============================================================

PyOscManager::PyOscManager() {
    std::lock_guard<std::mutex> lock{mutex};

    // TODO: Read from spec file. Also don't forget to check valid ranges
    for (int i = 7000; i < 8080; ++i) {

        allowed_ports.emplace_back(i);
        available_ports.emplace_back(i);
    }
}

bool PyOscManager::block_port(int port) {
    if (port_is_available(port)) {
        available_ports.erase(std::remove(available_ports.begin(), available_ports.end(), port)
                              , available_ports.end());
        return true;
    }

    return false;
}

bool PyOscManager::unblock_port(int port) {
    if (port_is_allowed(port) && !port_is_available(port)) {
        available_ports.emplace_back(port);
        return true;
    }
    return false;
}

bool PyOscManager::port_is_allowed(int port) {
    return std::find(allowed_ports.begin(), allowed_ports.end(), port) == allowed_ports.end();
}

bool PyOscManager::port_is_available(int port) {
    return std::find(available_ports.begin(), available_ports.end(), port) == available_ports.end();
}

bool PyOscManager::port_is_bound(int port) {
    try {
        auto socket = std::make_unique<UdpSocket>();
        socket->Bind(IpEndpointName(IpEndpointName::ANY_ADDRESS, port));
    } catch (std::runtime_error& e) {
        return true;
    }
    return false;
}

bool PyOscManager::name_exists(const std::string& object_name) {
    return get_object(object_name) != nullptr;
}


