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
    virtual bool initialize(std::string& init_message) = 0;


    /**
     * sends a message if the object has been initialized
     * @throws: std::runtime_error if `initialize` hasn't been called
     */
    virtual bool send(OscSendMessage& msg) = 0;


    /**
     * @throws: std::runtime_error if `initialize` hasn't been called
     */
    virtual std::vector<c74::min::atoms> receive(const std::string& address) = 0;


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

    const std::string& get_status_address() const;

protected:
    Status status;
    const std::string status_address;
    bool initialized = false;
    const std::string address;

    Status read_server_status();

    static Status parse_status(c74::min::atoms& msg);

    static std::string format_full_name(const std::string& name, const std::string& parent_name);

    static std::string format_address(const std::string& base_address, const std::string& parent);

private:
    std::optional<std::chrono::time_point<std::chrono::system_clock> > last_response;

    const std::string name;


};

// =========================================================================================



BaseOscObject::BaseOscObject(const std::string& name
                             , const std::string& address
                             , const std::string& status_address)
        : name(name)
          , address(address)
          , status_address(format_address(status_address, address))
          , status(Status::offline) {
}


Status BaseOscObject::parse_status(c74::min::atoms& msg) {
    if (msg.size() != 1) {
        return Status::invalid_status;
    }

    int status_code;

    if (msg[0].type() == c74::min::message_type::int_argument) {
        status_code = static_cast<int>(msg[0]);
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
        return status;
    } else {
        // initialize has been called but has not yet received any response from the server
        if (!last_response) {
            status = Status::initializing;
            last_response = std::chrono::system_clock::now();
            return status;
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


const std::string& BaseOscObject::get_address() const {
    return address;
}

const std::string& BaseOscObject::get_status_address() const {
    return status_address;
}

std::string BaseOscObject::format_full_name(const std::string& name, const std::string& parent_name) {
    return parent_name + "::" + name;
}

std::string BaseOscObject::format_address(const std::string& base_address, const std::string& parent) {
    std::stringstream ss;
    if (parent.find('/') != 0) {
        ss << "/";
    }
    ss << parent;

    if (base_address.find('/') != 0) {
        ss << "/";
    }
    ss << base_address;

    return ss.str();
}

#endif //PYOSC_PYOSC_BASE_H