#ifndef PYOSC_PYOSC_MANAGER_H
#define PYOSC_PYOSC_MANAGER_H

#include <iostream>
#include <vector>
#include "connector.h"

struct PortSpec {
    std::string ip;
    int send_port;
    int recv_port;

};

// Singleton class for managing OSC ports and object names
class PyOscManager {
public:

    // Instantiation is thread-safe (c++11): see https://stackoverflow.com/a/335746
    static PyOscManager& get_instance() {

        static PyOscManager instance; // Guaranteed to be destroyed.

        return instance; // Instantiated on first use.
    }

    PyOscManager(PyOscManager const&) = delete;

    void operator=(PyOscManager const&) = delete;

    bool add_object(std::string name) {
        std::lock_guard<std::mutex> lock{mutex};
        if (std::find(object_names.begin(), object_names.end(), name) != object_names.end()) {
            return false;
        }
        object_names.emplace_back(name);
        return true;
    }


    /**
     * @raises std::out_of_range if no ports are available
     *         std::runtime_error if fails to connect to port or if a connector is already linked to the `object_name`
     */
    std::shared_ptr<Connector>
    initialize_connector(std::string& object_name, std::optional<PortSpec> port_spec = std::nullopt) {
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

    /**
     *
     * @return connector associated with name or nullptr if not found
     */
    std::shared_ptr<Connector> get_connector(std::string& owner) {
        std::lock_guard<std::mutex> lock{mutex};
        auto it = connectors.find(owner);
        if (it != connectors.end()) {
            return it->second;
        }
        return nullptr;
    }

    bool delete_object(std::string& name) {
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


private:

    std::vector<int> allowed_ports;
    std::vector<int> available_ports;   // not guaranteed to be available, just not taken by this system
    std::vector<std::string> object_names;
    std::map<std::string, std::shared_ptr<Connector>> connectors;

    std::mutex mutex;

    PyOscManager() {
        std::lock_guard<std::mutex> lock{mutex};

        // TODO: Read from spec file. Also don't forget to check valid ranges
        for (int i = 7000; i < 8080; ++i) {

            allowed_ports.emplace_back(i);
            available_ports.emplace_back(i);
        }
    }

    bool block_port(int port) {
        if (port_is_available(port)) {
            available_ports.erase(std::remove(available_ports.begin(), available_ports.end(), port), available_ports.end());
            return true;
        }

        return false;
    }

    bool unblock_port(int port) {
        if (port_is_allowed(port) && !port_is_available(port)) {
            available_ports.emplace_back(port);
            return true;
        }
        return false;
    }

    bool port_is_allowed(int port) {
        return std::find(allowed_ports.begin(), allowed_ports.end(), port) == allowed_ports.end();
    }

    bool port_is_available(int port) {
        return  std::find(available_ports.begin(), available_ports.end(), port) == available_ports.end();
    }

    static bool port_is_bound(int port) {
        try {
            auto socket = std::make_unique<UdpSocket>();
            socket->Bind(IpEndpointName(IpEndpointName::ANY_ADDRESS, port));
        } catch (std::runtime_error& e) {
            return true;
        }
        return false;
    }

    bool name_exists(std::string& object_name) {
        return std::find(object_names.begin(), object_names.end(), object_name) != object_names.end();
    }

    bool connector_exists(std::string& object_name) {
        return connectors.count(object_name) != 0;
    }

    int pop_available_port() {
        int send_port = available_ports.at(0);
        available_ports.erase(available_ports.begin());
        return send_port;
    }


};

#endif //PYOSC_PYOSC_MANAGER_H