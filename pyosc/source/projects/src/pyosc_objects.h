#ifndef PYOSC_PYOSC_OBJECTS_H
#define PYOSC_PYOSC_OBJECTS_H

#include <utility>

#include "pyosc_base.h"

// TODO: Mutexes in Server/Thread/Remote
// TODO: Mutexes in Server/Thread/Remote
// TODO: Mutexes in Server/Thread/Remote

class Server : public BaseOscObject {
public:
    /**
     * @throws: std::runtime_error if name already exists.
     */
    Server(const std::string& name
           , const std::string& status_address
           , std::optional<std::string>  termination_message
           , std::unique_ptr<Connector> connector)
            : BaseOscObject(name, "/" + name, status_address)
              , termination_message(termination_message)
              , connector(std::move(connector)) {}

    bool initialize(std::string& init_message) override {
        if (initialized) {
            throw std::runtime_error("object has already been initialized");
        }

        initialized = true;
        return initialized;
    }

    bool send(OscSendMessage& msg) override {
        if (status == Status::ready) {
            connector->send(msg);
        }
        return false;
    }

    std::vector<osc::ReceivedMessage> receive(const std::string& address) override {
        if (status != Status::deleted) {
            return connector->receive(address);
        }
        return {};
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

    void flag_for_deletion() override {
        status = Status::deleted;
        connector.reset();  // delete socket to allow reallocation of ports
    }

    int get_send_port() {
        return connector->get_send_port();
    }

    int get_recv_port() {
        return connector->get_recv_port();
    }

private:
    std::unique_ptr<Connector> connector;
    std::optional<std::string> termination_message;
};


// ============================================================================================================


// TODO: Thread and Remote are more or less identical apart from their send and receive. Refactor at some point.
class Thread : public BaseOscObject {
public:
    Thread(const std::string& name
           , const std::string& status_address
           , const std::string& termination_message
           , std::unique_ptr<Connector> connector
           , const std::string& parent_name
           , std::shared_ptr<BaseOscObject> parent_object = nullptr)
            : BaseOscObject(name, "/" + name, status_address)
              , termination_message(termination_message)
              , connector(std::move(connector))
              , parent_name(parent_name)
              , parent(std::move(parent_object))
              {}


    bool initialize(std::string& init_message) override {
        if (initialized) {
            throw std::runtime_error("object is already initialized");
        }

        update_status();

        if (status == Status::uninitialized) {
            OscSendMessage msg{address};
            msg.add_string(init_message);

            bool res = parent->send(msg);
            initialized = res;
            return initialized;
        }

        return false;
    }

    bool send(OscSendMessage& msg) override {
        if (status == Status::ready) {
            connector->send(msg);
            return true;
        }

        return false;
    }

    std::vector<osc::ReceivedMessage> receive(const std::string& address) override {
        return connector->receive(address);
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

    void flag_for_deletion() override {
        status = Status::deleted;
        connector.reset();  // delete socket to allow reallocation of ports
    }

    int get_send_port() {
        return connector->get_send_port();
    }

    int get_recv_port() {
        return connector->get_recv_port();
    }

    void set_parent(const std::shared_ptr<BaseOscObject>& parent) {
        Thread::parent = parent;
    }

    const std::string& get_parent_name() const {
        return parent_name;
    }



private:

    const std::string parent_name;
    std::shared_ptr<BaseOscObject> parent;

    const std::string termination_message;

    std::unique_ptr<Connector> connector;


};

// ============================================================================================================

class Remote : public BaseOscObject {
public:
    Remote(const std::string& name
           , const std::string& status_base_address
           , const std::string& termination_message
           , const std::string& parent_name
           , std::shared_ptr<BaseOscObject> parent_object = nullptr)
            : BaseOscObject(format_full_name(name, parent_name)
                            , format_address(name, parent_name)
                            , status_base_address)
              , termination_message(termination_message)
              , base_name(name)
              , parent_name(parent_name)
              , parent(std::move(parent_object))
              {}


    bool initialize(std::string& init_message) override {
        if (initialized) {
            throw std::runtime_error("object is already initialized");
        }

        update_status();

        if (status == Status::uninitialized) {
            OscSendMessage msg{address};
            msg.add_string(init_message);

            bool res = parent->send(msg);
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

    void flag_for_deletion() override {
        status = Status::deleted;
    }

private:
    const std::string base_name;
    const std::string parent_name;
    std::shared_ptr<BaseOscObject> parent;

    const std::string termination_message;

};

#endif //PYOSC_PYOSC_OBJECTS_H
