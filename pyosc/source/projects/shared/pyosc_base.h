#ifndef PYOSC_PYOSC_BASE_H
#define PYOSC_PYOSC_BASE_H

#include <iostream>
#include <utility>

#include <vector>
#include <sstream>
#include "connector.h"


// TODO: These are only the good statuses, handle bad statuses too (`parent_no_response`, `no_response`, etc.)
enum class Status {
    invalid_status = -1         // status outside valid range
    , offline = 0               // status when creating the object
    , parent_obj_missing = 1    // this object has an `parent_obj_name` which hasn't been created yet
    , parent_obj_not_ready = 2  // this object has an `parent_connection` which returns a non-ready status
    , uninitialized = 3         // both `parent_connection` and `runtime_connection` exists and return ready status
    , initializing = 4          // `init` has been called but server object has not returned ready yet
    , ready = 5                 // server object returns ready
    , no_response = 6           // server didn't respons to status message within timeout limit
    , deleted = 7               // flagged for deletion since owning max object has been deleted
};


struct PortSpec {
    int send_port;
    int recv_port;

};


class BaseOscObject : public std::enable_shared_from_this<BaseOscObject> {
public:

    const static Status MIN_STATUS = Status::offline;
    const static Status MAX_STATUS = Status::deleted;
    const static long TIMEOUT_SECONDS = 5;


    /**
     * @throws: std::runtime_error if name already exists
     */
    BaseOscObject(const std::string& name
                  , const std::string& status_address);


    /**
     * initializes the object on the server
     */
    virtual bool initialize(OscSendMessage& init_message) = 0;


    /**
     * sends a message if the object has been initialized
     * @throws: std::runtime_error if `initialize` hasn't been called
     */
    virtual bool send(OscSendMessage& msg) = 0;


    /**
     * @throws: std::runtime_error if `initialize` hasn't been called
     */
    virtual std::vector<osc::ReceivedMessage> receive(const std::string& address) = 0;


    /**
     *  Terminates (removes) the object on the server without removing the local max object
     */
    virtual void terminate() = 0;

    /**
     *  Max object deleted. Since this object may be used by other objects, it cannot be directly deleted at the same
     *  time as the owning max object, but will rather be flagged for deletion manually
     */
     void delete_object();


    virtual void update_status() = 0;


    [[nodiscard]] Status get_status() {
        return status;
    }

    const std::string& get_name() const {
        return name;
    }

    const std::string& get_address() const;


protected:
    Status status;
    const std::string status_address;
    bool initialized = false;
    const std::string address;


    static std::shared_ptr<BaseOscObject> get_object(std::string& object_name);


    Connector create_connector(std::optional<PortSpec> port_spec = std::nullopt
                               , std::optional<std::string> ip = std::nullopt);


    Status read_server_status();


    static Status parse_status(osc::ReceivedMessage& msg);

private:
    std::optional<std::chrono::time_point<std::chrono::system_clock> > last_response;

    const std::string name;






};

// =========================================================================================


// Singleton class for managing OSC ports and object names
class PyOscManager {
public:

    // Instantiation is thread-safe (c++11): see https://stackoverflow.com/a/335746
    static PyOscManager& get_instance();


    PyOscManager(PyOscManager const&) = delete;


    void operator=(PyOscManager const&) = delete;


    bool add_object(std::shared_ptr<BaseOscObject> object);


    std::shared_ptr<BaseOscObject> get_object();


    bool remove_object(std::string& name);


    /**
     * @raises std::out_of_range if no ports are available
     *         std::runtime_error if fails to connect to port or if a connector is already linked to the `object_name`
     */
    Connector new_connector(const std::string& ip, std::optional<PortSpec> port_spec = std::nullopt);

    bool reallocate_connector(Connector& connector);

//    /**
//     *
//     * @return connector associated with name or nullptr if not found
//     */
//    std::shared_ptr<Connector> get_connector(std::string& owner) {
//        std::lock_guard<std::mutex> lock{mutex};
//        auto it = connectors.find(owner);
//        if (it != connectors.end()) {
//            return it->second;
//        }
//        return nullptr;
//    }



private:

    std::vector<int> allowed_ports;
    std::vector<int> available_ports;   // not guaranteed to be available, just not taken by this system
    std::vector<std::string> object_names;
    std::map<std::string, std::shared_ptr<Connector>> connectors;

    std::mutex mutex;

    PyOscManager();

    bool block_port(int port);

    bool unblock_port(int port);

    bool port_is_allowed(int port);

    bool port_is_available(int port);

    static bool port_is_bound(int port);

    bool name_exists(std::string& object_name);

    bool connector_exists(std::string& object_name);

    int pop_available_port();


};


#endif //PYOSC_PYOSC_BASE_H