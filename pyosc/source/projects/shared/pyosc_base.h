#ifndef PYOSC_PYOSC_BASE_H
#define PYOSC_PYOSC_BASE_H

#include <iostream>
#include <utility>

#include <vector>
#include "connector.h"

// TODO: These are only the good statuses, handle bad statuses too (`parent_no_response`, `no_response`, etc.)
enum class Status {
    invalid_status = -3         // status outside valid range
    , not_applicable = -2       // user has opted for no poll_status address
    , init_obj_missing = -1     // this object has an `init_object_name` which hasn't been created yet
    , init_obj_not_ready = 0    // this object has an `init_object` which returns a non-ready status
    , runtime_obj_missing = 1   // this object has a `runtime_object_name` which hasn't been created yet
    , runtime_obj_not_ready = 2 // this object has a `runtime_object` which return a non-ready status
    , uninitialized = 3         // both `init_object` and `runtime_object` exists and return ready status
    , initializing = 4          // `init` has been called but server object has not returned ready yet
    , ready = 5                 // server object returns ready
    , no_response = 6
};


struct PortSpec {
    int send_port;
    int recv_port;

};

class BaseOscObject {
public:

    const static Status MIN_STATUS = Status::invalid_status;
    const static Status MAX_STATUS = Status::ready;
    const static long TIMEOUT_SECONDS = 5;

    BaseOscObject(const std::string name
                  , const std::string base_address
                  , const std::string ip
                  , const std::optional<PortSpec> port_spec
                  , const std::optional<std::string> status_base_address
                  , const std::optional<std::string> init_object_name
                  , const std::optional<std::string> runtime_object_name)
            : name(name)
              , base_address(base_address)
              , ip(ip)
              , port_spec(port_spec)
              , status_base_address(status_base_address) {}


    void initialize() {
        // TODO: Mutex - lock guard or try_lock?

    }

    /**
     *
     * @throws: std::runtime_error if `initialize` hasn't been called
     */
    bool send(OscSendMessage& msg) {
        // TODO: Mutex - lock guard or try_lock?
        if (!runtime_object) {
            throw std::runtime_error("object not initialized");
        } else {
            throw std::runtime_error("Not implemented yet");
        }
    }

    /**
     *
     * @throws: std::runtime_error if `initialize` hasn't been called
     */
    std::vector<osc::ReceivedMessage> receive(std::string& address) {
        // TODO: Mutex - lock guard or try_lock?
        if (!runtime_object) {
            throw std::runtime_error("object not initialized");
        } else {
            throw std::runtime_error("Not implemented yet"); // TODO: Implement
        }
    }

    void terminate() {
        throw std::runtime_error("Not implemented yet"); // TODO: Implement
    }

    void update_status() {
        // TODO: Mutex

        // User has opted to not use `poll_status`
        if (!status_address) {
            status = Status::not_applicable;
        }
            // the object is initialized through an object that has not been created yet
        else if (init_object_name && !init_object) {
            status = Status::init_obj_missing;
        }
            // the object is initialized through an object that isn't ready yet
        else if (init_object && init_object->get_status() != Status::ready) {
            status = Status::init_obj_not_ready;
        }
            // the object communicates through an object that has not been created yet
        else if (runtime_object_name && !runtime_object) {
            status = Status::runtime_obj_missing;
        }
            // the object communicates through an object that isn't ready yet
        else if (runtime_object && runtime_object->get_status() != Status::ready) {
            status = Status::runtime_obj_not_ready;
        }
            // both `init_object` and `runtime_object` exist and return `Status::ready`
            //  but this object hasn't called initialize()
        else if (!initialized) {
            status = Status::uninitialized;
        } else {
            // initialize has been called but has not yet received any response from the server
            if (!last_response) {
                status = Status::initializing;
            }

            // This should never happen: `status_address` should've been set at initialize()
            if (!status_address) {
                set_status_address();
            }

            // Read status messages from receiver
            auto new_status_values = receive(status_address.value());

            // If status message received: set status to received message and update time
            if (!new_status_values.empty()) {
                auto last_status_candidate = new_status_values.back();
                status = parse_status(last_status_candidate);
                last_response = std::chrono::system_clock::now();
                // if
            } else if (std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now() - last_response.value()
            ).count() > TIMEOUT_SECONDS) {
                status = Status::no_response;
            }
        }
    }

    [[nodiscard]] Status get_status() {
        return status;
    }


private:
    Status status = Status::runtime_obj_not_ready;
    std::optional<std::chrono::time_point<std::chrono::system_clock> > last_response;


    const std::string name;

    const std::string ip;
    const std::optional<PortSpec> port_spec;

    const std::string base_address;
    const std::optional<std::string> status_base_address;


    std::optional<std::string> address;
    std::optional<std::string> status_address;

    const std::optional<std::string> init_object_name;
    const std::optional<std::string> runtime_object_name;

    std::shared_ptr<BaseOscObject> init_object;
    std::shared_ptr<BaseOscObject> runtime_object;

    bool initialized = false;


//    bool is_connected() {
//        return runtime_object && runtime_object->is_connected()
//    }

    void set_status_address() {
        std::runtime_error("not implemented yet"); // TODO
    }

    Status parse_status(osc::ReceivedMessage& msg) {
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
            available_ports.erase(std::remove(available_ports.begin(), available_ports.end(), port)
                                  , available_ports.end());
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
        return std::find(available_ports.begin(), available_ports.end(), port) == available_ports.end();
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


#endif //PYOSC_PYOSC_BASE_H