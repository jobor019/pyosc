#ifndef PYOSC_PYOSC_BASE_H
#define PYOSC_PYOSC_BASE_H

#include <iostream>
#include <utility>

#include <vector>
#include <sstream>
#include "connector.h"


enum class Status {
    invalid_status = -1         // status outside valid range
    , offline = 0               // status when creating the object
    , parent_obj_missing = 1    // this object has an `parent_obj_name` which hasn't been created yet
    , parent_obj_not_ready = 2  // this object has an `parent_connection` which returns a non-ready status
    , uninitialized = 3         // both `parent_connection` and `runtime_connection` exists and return ready status
    , initializing = 4          // `init` has been called but server object has not returned ready yet
    , ready = 5                 // server object returns ready
    , no_response = 6           // server didn't respond to status message within timeout limit
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
                  , const std::string& address
                  , const std::string& status_address);


    /**
     * initializes the object on the server
     */
    virtual bool initialize(OscSendMessage init_message) = 0;


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
     *  If terminated, the object can be restarted with the `initialize` function
     */
    virtual void terminate() = 0;

    /**
     *  Max object deleted. Since this object may be used by other objects, it cannot be directly deleted at the same
     *  time as the owning max object, but will rather be flagged for deletion manually
     *
     *  If flagged for deletion, the object will be partially destroyed and therefore cannot be used again.
     */
    virtual void flag_for_deletion() = 0;


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

    Status read_server_status();

    static Status parse_status(osc::ReceivedMessage& msg);

private:
    std::optional<std::chrono::time_point<std::chrono::system_clock> > last_response;

    const std::string name;


};

// =========================================================================================





#endif //PYOSC_PYOSC_BASE_H