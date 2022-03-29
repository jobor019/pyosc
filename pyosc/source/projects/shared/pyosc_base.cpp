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
        return Status::uninitialized;
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
                                                                    last_response.value()).count() > TIMEOUT_SECONDS) {
            status = Status::no_response;
        }
    }
}

Connector BaseOscObject::create_connector(std::optional<PortSpec> port_spec, std::optional<std::string> ip) {
    // TODO
}

std::shared_ptr<BaseOscObject> BaseOscObject::get_object(std::string& object_name) {
    // TODO
}


void BaseOscObject::delete_object() {
    terminate();
    status = Status::deleted;
}


const std::string& BaseOscObject::get_address() const {
    return address;
}


// Old update status:
//    {
//        // TODO: Mutex
//        // TODO: Should probably also handle case if `init` or `runtime` are terminated/deleted
//
//        // User has opted to not use `poll_status`
//        if (!status_address) {
//            status = Status::not_applicable;
//        }
//            // the object is initialized through an object that has not been created yet
//        else if (parent_obj_name && !parent_connection) {
//            status = Status::parent_obj_missing;
//        }
//            // the object is initialized through an object that isn't ready yet
//        else if (parent_connection && parent_connection->get_status() != Status::ready) {
//            status = Status::parent_obj_not_ready;
//        }
//            // the object communicates through an object that has not been created yet
//        else if (runtime_obj_name && !runtime_connection) {
//            status = Status::runtime_obj_missing;
//        }
//            // the object communicates through an object that isn't ready yet
//        else if (runtime_connection && runtime_connection->get_status() != Status::ready) {
//            status = Status::runtime_obj_not_ready;
//        }
//            // both `parent_connection` and `runtime_connection` exist and return `Status::ready`
//            //  but this object hasn't called initialize()
//        else if (!initialized) {
//            status = Status::uninitialized;
//        } else {
//            // initialize has been called but has not yet received any response from the server
//            if (!last_response) {
//                status = Status::initializing;
//            }
//
//            // This should never happen: `status_address` should've been set at initialize()
//            if (!status_address) {
//                set_status_address();
//            }
//
//            // Read status messages from receiver
//            auto new_status_values = receive(status_address.value());
//
//            // If status message received: set status to received message and update time
//            if (!new_status_values.empty()) {
//                auto last_status_candidate = new_status_values.back();
//                status = parse_status(last_status_candidate);
//                last_response = std::chrono::system_clock::now();
//                // if
//            } else if (std::chrono::duration_cast<std::chrono::seconds>(
//                    std::chrono::system_clock::now() - last_response.value()
//            ).count() > TIMEOUT_SECONDS) {
//                status = Status::no_response;
//            }
//        }
//    }

//  ================================================================================================= //
//  == PyOscManager ==
//  ================================================================================================= //


bool PyOscManager::add_object(std::shared_ptr<BaseOscObject> object) {
    std::lock_guard<std::mutex> lock{mutex};
    std::string name = object->get_name();
    if (std::find(object_names.begin(), object_names.end(), name) != object_names.end()) {
        return false;
    }
    object_names.emplace_back(name);
    return true;
}

std::shared_ptr<BaseOscObject> PyOscManager::get_object() {
    return nullptr;     // TODO
}

bool PyOscManager::remove_object(std::string& name) {
    std::lock_guard<std::mutex> lock{mutex};
    object_names.erase(std::remove(object_names.begin(), object_names.end(), name), object_names.end());

    auto it = connectors.find(name);
    if (it != connectors.end()) {
        it->second->terminate();
        unblock_port(it->second->get_send_port());
        unblock_port(it->second->get_recv_port());
        connectors.erase(it);
        return true;
    }
    return false;
}

PyOscManager& PyOscManager::get_instance() {
    static PyOscManager instance; // Guaranteed to be destroyed.
    return instance; // Instantiated on first use.
}

Connector PyOscManager::new_connector(std::string& object_name, std::optional<PortSpec> port_spec) {
    std::lock_guard<std::mutex> lock{mutex};
    // TODO: Start by checking whether port is available or not
    //  (not in `available_ports` but by attempting to connect to the port)


    // Checking object names
    if (std::find(object_names.begin(), object_names.end(), object_name) == object_names.end()) {
        object_names.emplace_back(object_name);
    }

    // Checking existing connectors
    if (connectors.count(object_name) != 0) {
        throw std::runtime_error("There is already a connector linked to the object with the given name");
    }

    int send_port, recv_port;
    std::string ip;

    // Automatic port assignment
    if (!port_spec) {

        send_port = pop_available_port();

        // TODO: This solution will remove ports from the availability without readding them if failing,
        //  effectively blocking them forever. Solve
        while (port_is_bound(send_port)) {
            send_port = pop_available_port();
        }

        recv_port = pop_available_port();

        while (port_is_bound(recv_port)) {
            recv_port = pop_available_port();
        }

        ip = "127.0.0.1";

        // Manual port assignment
    } else {
        // Should not check if ports are available since this case is manually handled by the user. throw if bound
        send_port = port_spec->send_port;
        recv_port = port_spec->recv_port;
        ip = port_spec->ip;
    }

    auto connector = std::make_shared<Connector>(ip, send_port, recv_port);
    connectors.insert(std::pair<std::string, std::shared_ptr<Connector>>(object_name, connector));

    block_port(send_port);
    block_port(recv_port);

    return connector;

}

bool PyOscManager::reallocate_connector() {
    // TODO
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

bool PyOscManager::name_exists(std::string& object_name) {
    return std::find(object_names.begin(), object_names.end(), object_name) != object_names.end();
}

bool PyOscManager::connector_exists(std::string& object_name) {
    return connectors.count(object_name) != 0;
}

int PyOscManager::pop_available_port() {
    int send_port = available_ports.at(0);
    available_ports.erase(available_ports.begin());
    return send_port;
}
