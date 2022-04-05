
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

    std::shared_ptr<Thread> new_thread(const std::string& name, const std::string& status_address, const std::string& ip
                                       , const std::optional<PortSpec> port_spec);

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




#endif //PYOSC_PYOSC_MANAGER_H
