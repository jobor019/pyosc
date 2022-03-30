#ifndef PYOSC_PYOSC_OBJECTS_H
#define PYOSC_PYOSC_OBJECTS_H

#include "pyosc_base.h"

class Server : public BaseOscObject {
public:
    /**
     * @throws: std::runtime_error if name already exists.
     */
    Server(const std::string& name
           , const std::string status_address
           , const std::optional<std::string> termination_message
           , const std::string& ip
           , const std::optional<PortSpec> port_spec)
            : BaseOscObject(name, status_address)
              , termination_message(termination_message)
            // Note: if BaseOscObject ctor fails, connector will never be initialized so no need to clean this up
              , connector(create_connector(ip, port_spec)) {}

    // TODO: This doesn't work very well with the interface - shouldn't pass OscSendMessage
    bool initialize(OscSendMessage& init_message) override {
        initialized = true;
        return initialized;
    }

    bool send(OscSendMessage& msg) override {
        if (status == Status::ready) {
            connector.send(msg);
        }
        return false;
    }

    std::vector<osc::ReceivedMessage> receive(const std::string& address) override {
        return connector.receive(address);
    }


    void update_status() override {
        if (status == Status::deleted) {
            return;
        }

        status = read_server_status();
    }

    void terminate() override {
        // TODO: Doesn't handle the case of a temporary timeout properly
        if (termination_message && status == Status::ready) {
            auto msg = OscSendMessage(address);
            msg.add_string(*termination_message);
            send(msg);

        }
        // TODO: It would be better to wait for response from server confirming that it was terminated
        initialized = false;
    }

private:
    Connector connector;
    std::optional<std::string> termination_message;
};


// ============================================================================================================


// TODO: Thread and Remote are more or less identical apart from their send and receive. Refactor.
class Thread : public BaseOscObject {
public:
    Thread(const std::string& name
           , const std::string& status_address
           , const std::string& termination_message
           , const std::string& ip
           , const std::optional<PortSpec> port_spec
           , const std::string& parent_name)
            : BaseOscObject(name, status_address)
              , parent_name(parent_name)
              , termination_message(termination_message)
            // Note: if BaseOscObject ctor fails, connector will never be initialized so no need to clean this up
              , connector(create_connector(ip, port_spec)) {}

    // TODO: dtor to reallocate ports, delete copy/move constr/asgn.

    bool initialize(OscSendMessage& init_message) override {
        if (initialized) {
            throw std::runtime_error("object is already initialized");
        }

        update_status();

        if (status == Status::uninitialized) {
            bool res = parent->send(init_message);
            initialized = res;
            return initialized;
        }

        return false;
    }

    bool send(OscSendMessage& msg) override {
        if (status == Status::ready) {
            connector.send(msg);
            return true;
        }

        return false;
    }

    std::vector<osc::ReceivedMessage> receive(const std::string& address) override {
        return connector.receive(address);
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
        if (parent && status == Status::ready) {
            auto msg = OscSendMessage(address);
            msg.add_string(termination_message);
            parent->send(msg);
        }
        // TODO: Should rather wait for response from server
        initialized = false;
    }

private:

    const std::string parent_name;
    std::shared_ptr<BaseOscObject> parent;

    const std::string termination_message;

    Connector connector;


};

// ============================================================================================================

class Remote : public BaseOscObject {
public:
    Remote(const std::string& name
           , const std::string& status_base_address
           , const std::string& termination_message
           , const std::string& parent_name)
            : BaseOscObject(name
                            , format_status_address(status_base_address, parent_name))
              , termination_message(termination_message) {}


    bool initialize(OscSendMessage& init_message) override {
        if (initialized) {
            throw std::runtime_error("object is already initialized");
        }

        update_status();

        if (status == Status::uninitialized) {
            bool res = parent->send(init_message);
            initialized = res;
            return initialized;
        }
        return false;
    }


    bool send(OscSendMessage& msg) override {
        if (status == Status::ready) {
            return parent->send(msg);
        }
        return false;
    }

    std::vector<osc::ReceivedMessage> receive(const std::string& address) override {
        return parent->receive(address);
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
        if (parent && status == Status::ready) {
            auto msg = OscSendMessage(address);
            msg.add_string(termination_message);
            parent->send(msg);
        }
        // TODO: Should rather wait for response from server
        initialized = false;
    }

private:
    const std::string base_name;
    const std::string parent_name;
    std::shared_ptr<BaseOscObject> parent;

    const std::string termination_message;


    static std::string format_address(std::string& name, std::string& parent_name) {
        return parent_name + "::" + name;
    }

    static std::string format_status_address(const std::string& status_base_address
                                             , const std::string& parent_name) {
        return "/" + parent_name + status_base_address;
    }
};

#endif //PYOSC_PYOSC_OBJECTS_H
