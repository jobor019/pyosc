#ifndef PYOSC_PYOSC_BASE_H
#define PYOSC_PYOSC_BASE_H

#include <iostream>
#include <utility>

#include <vector>
#include <regex>
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
    virtual bool initialize(std::unique_ptr<OscSendMessage> init_message, std::unique_ptr<OscSendMessage> termination_message) = 0;


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

    virtual int get_send_port() = 0;

    virtual int get_recv_port() = 0;

    virtual std::string type() = 0;

    virtual const std::string& get_initialization_address() = 0;

    [[nodiscard]] const Status& get_status() {
        return status;
    }

    const std::string& get_name() const {
        return name;
    }

    const std::string& get_address() const;

    const std::string& get_status_address() const;

    bool is_initialized() const;

    static std::string format_full_name(const std::string& name, const std::string& parent_name);

    static std::string format_address(const std::string& base_address, const std::string& parent);

    static std::string address_from_full_name(const std::string& name);

protected:
    Status status;
    const std::string status_address;
    bool initialized = false;
    const std::string address;
    std::unique_ptr<OscSendMessage> termination_message;

    Status read_server_status();

    static Status parse_status(c74::min::atoms& msg);



private:
    std::optional<std::chrono::time_point<std::chrono::system_clock> > last_response;

    const std::string name;


};

// =========================================================================================


class BaseWithParent : public BaseOscObject {
public:
    BaseWithParent(const std::string& name
                   , const std::string& address
                   , const std::string& status_address
                   , const std::string& parent_name)
            : BaseOscObject(name, address, status_address)
              , parent_name(parent_name) {}


    bool initialize(std::unique_ptr<OscSendMessage> init_message
                    , std::unique_ptr<OscSendMessage> termination_message) override {
        if (initialized) {
            throw std::runtime_error("object is already initialized");
        }

        if (parent_name.empty()) {
            throw std::runtime_error("a parent name must be provided before initializing");
        }

        update_status();

        if (status == Status::uninitialized) {
            // If send fails, this message will regardless be overwritten at next call to initialize
            this->termination_message = std::move(termination_message);

            bool res = parent->send(*init_message);
            initialized = res;
            return initialized;
        }

        return false;
    }


    void update_status() override {
        if (status == Status::deleted) {
            return;

        } else if (!parent) {
            status = Status::parent_obj_missing;

        } else if (parent->get_status() != Status::ready) {

            if (parent->get_status() == Status::deleted) {
                parent.reset();
                status = Status::parent_obj_missing;

            } else {
                status = Status::parent_obj_not_ready;
            }

        } else {
            status = read_server_status();
        }
    }

    void terminate() override {
        // TODO: Doesn't handle the case of a temporary timeout properly
        if (parent && status == Status::ready && termination_message) {
            parent->send(*termination_message);
        }
        // TODO: Should rather wait for response from server. Also if above is false, need to handle this case properly!
        initialized = false;
    }


    void set_parent(const std::shared_ptr<BaseOscObject>& new_parent) {
        BaseWithParent::parent = new_parent;
    }


    const std::string& get_parent_name() const {
        return parent_name;
    }

    const std::string& get_initialization_address() override {
        if (!parent) {
            throw std::runtime_error("no parent exists");
        }

        return parent->get_address();
    }


protected:
    const std::string parent_name;
    std::shared_ptr<BaseOscObject> parent;
};


#endif //PYOSC_PYOSC_BASE_H