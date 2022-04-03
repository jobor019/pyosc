
#ifndef PYOSC_PYOSC_MANAGER_H
#define PYOSC_PYOSC_MANAGER_H

#include <iostream>

#include "connector.h"
#include "pyosc_base.h"
#include "pyosc_objects.h"

// Singleton class for managing OSC ports and object names
class PyOscManager {
public:

    // Instantiation is thread-safe (c++11): see https://stackoverflow.com/a/335746
    static PyOscManager& get_instance();


    PyOscManager(PyOscManager const&) = delete;


    void operator=(PyOscManager const&) = delete;


    std::shared_ptr<Server> new_server(const std::string& name
                                       , const std::string& status_address
                                       , const std::optional<std::string>& termination_message
                                       , const std::string& ip
                                       , std::optional<PortSpec> port_spec);

    std::shared_ptr<Thread> new_thread(const std::string& name
                                       , const std::string& status_address
                                       , const std::string& termination_message
                                       , const std::string& ip
                                       , std::optional<PortSpec> port_spec
                                       , const std::string& parent_name);

    std::shared_ptr<Remote> new_remote(const std::string& name
                                       , const std::string& status_base_address
                                       , const std::string& termination_message
                                       , const std::string& parent_name);


    std::shared_ptr<BaseOscObject> get_object(const std::string& name);


    void remove_object(const std::string& name);


    unsigned long num_objects();


private:

    std::vector<int> allowed_ports;
    std::vector<int> available_ports;   // not guaranteed to be available, just not taken by this system
    std::vector<std::shared_ptr<BaseOscObject> > objects;

    std::mutex mutex;

    PyOscManager();

    bool block_port(int port);

    bool unblock_port(int port);

    bool port_is_allowed(int port);

    bool port_is_available(int port);

    static bool port_is_bound(int port);

    bool name_exists(const std::string& object_name);

    std::shared_ptr<BaseOscObject> get_object_lock_free(const std::string& object_name);

    std::unique_ptr<Connector> create_connector_lock_free(const std::string& ip
                                                          , std::optional<PortSpec> port_spec = std::nullopt);
};

PyOscManager& PyOscManager::get_instance() {
    static PyOscManager instance; // Guaranteed to be destroyed.
    return instance; // Instantiated on first use.
}

std::shared_ptr<Server> PyOscManager::new_server(const std::string& name
                                                 , const std::string& status_address
                                                 , const std::optional<std::string>& termination_message
                                                 , const std::string& ip
                                                 , const std::optional<PortSpec> port_spec) {
    std::lock_guard<std::mutex> lock{mutex};
    if (name_exists(name)) {
        throw std::runtime_error("an object with the name " + name + " already exists");
    }

    // Throws std::runtime_error if ports are taken or out of available ports
    auto connector = create_connector_lock_free(ip, port_spec);

    auto object = std::make_shared<Server>(name, status_address, termination_message, std::move(connector));
    objects.emplace_back(object);

    return object;
}

std::shared_ptr<Thread> PyOscManager::new_thread(const std::string& name, const std::string& status_address
                                                 , const std::string& termination_message, const std::string& ip
                                                 , const std::optional<PortSpec> port_spec
                                                 , const std::string& parent_name) {
    std::lock_guard<std::mutex> lock{mutex};
    if (name_exists(name)) {
        throw std::runtime_error("an object with the name " + name + " already exists");
    }

    // Throws std::runtime_error if ports are taken or out of available ports
    auto connector = create_connector_lock_free(ip, port_spec);

    auto parent_object = get_object_lock_free(parent_name); // nullptr if not created yet

    auto object = std::make_shared<Thread>(name, status_address, termination_message
                                           , std::move(connector), parent_name, parent_object);

    objects.emplace_back(object);

    return object;
}

std::shared_ptr<Remote> PyOscManager::new_remote(const std::string& name, const std::string& status_base_address
                                                 , const std::string& termination_message
                                                 , const std::string& parent_name) {
    std::lock_guard<std::mutex> lock{mutex};
    if (name_exists(name)) {
        throw std::runtime_error("an object with the name " + name + " already exists");
    }

    auto parent_object = get_object_lock_free(parent_name); // nullptr if not created yet

    auto object = std::make_shared<Remote>(name, status_base_address, termination_message
                                           , parent_name, parent_object);

    objects.emplace_back(object);

    return object;
}


//bool PyOscManager::add_object(std::shared_ptr<BaseOscObject> object) {
//    std::lock_guard<std::mutex> lock{mutex};
//    if (name_exists(object->get_name())) {
//        return false;
//    }
//
//    objects.emplace_back(object);
//    return true;
//}


std::shared_ptr<BaseOscObject> PyOscManager::get_object(const std::string& name) {
    std::lock_guard<std::mutex> lock{mutex};
    return get_object_lock_free(name);
}


void PyOscManager::remove_object(const std::string& name) {
    std::lock_guard<std::mutex> lock{mutex};

    objects.erase(std::remove_if(objects.begin(), objects.end(), [&name, this](std::shared_ptr<BaseOscObject>& obj) {
        if (obj->get_name() == name) {
            // If server: reallocate ports
            auto server = std::dynamic_pointer_cast<Server>(obj);
            if (server) {
                this->unblock_port(server->get_send_port());
                this->unblock_port(server->get_recv_port());
            }

            //If thread: reallocate ports
            auto thread = std::dynamic_pointer_cast<Thread>(obj);
            if (thread) {
                this->unblock_port(thread->get_send_port());
                this->unblock_port(thread->get_recv_port());
            }

            obj->flag_for_deletion();
            return true;
        }
        return false;
    }), objects.end());

}


//std::unique_ptr<Connector> PyOscManager::new_connector(const std::string& ip, std::optional<PortSpec> port_spec) {
//    std::lock_guard<std::mutex> lock{mutex};
//    return create_connector_lock_free(ip, port_spec);
//
//}



unsigned long PyOscManager::num_objects() {
    return objects.size();
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
    return std::find(allowed_ports.begin(), allowed_ports.end(), port) != allowed_ports.end();
}

bool PyOscManager::port_is_available(int port) {
    return std::find(available_ports.begin(), available_ports.end(), port) != available_ports.end();
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
    return get_object_lock_free(object_name) != nullptr;
}

std::shared_ptr<BaseOscObject> PyOscManager::get_object_lock_free(const std::string& object_name) {
    for (auto& obj: objects) {
        if (obj->get_name() == object_name) {
            return obj;
        }
    }
    return nullptr;
}

std::unique_ptr<Connector> PyOscManager::create_connector_lock_free(const std::string& ip
                                                                    , std::optional<PortSpec> port_spec) {
    int send_port = -1;
    int recv_port = -1;

    if (port_spec) {
        // Manual port assignment
        send_port = port_spec->send_port;
        recv_port = port_spec->recv_port;

        // No checks: up to the user to ensure that ports aren't used elsewhere. Will throw if recv_port is bound
        auto connector = std::make_unique<Connector>(ip, send_port, recv_port);

        block_port(connector->get_send_port());
        block_port(connector->get_recv_port());

        return std::move(connector);

    } else {
        // Automatic port assignment
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
            // deallocate send_port due to failure
            unblock_port(send_port);
            throw std::runtime_error("not enough ports available");
        }

        return std::make_unique<Connector>(ip, send_port, recv_port);

    }
}


#endif //PYOSC_PYOSC_MANAGER_H
