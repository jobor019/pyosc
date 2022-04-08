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
           , std::unique_ptr<Connector> connector)
            : BaseOscObject(name, "/" + name, status_address)
              , connector(std::move(connector)) {}

    bool initialize(std::unique_ptr<OscSendMessage> init_message
                    , std::unique_ptr<OscSendMessage> termination_message) override {
        if (initialized) {
            throw std::runtime_error("object has already been initialized");
        }

        Server::termination_message = std::move(termination_message);

        initialized = true;
        return initialized;
    }

    bool send(OscSendMessage& msg) override {
        if (status == Status::ready) {
            connector->send(msg);
            return true;
        }
        return false;
    }

    std::vector<c74::min::atoms> receive(const std::string& address) override {
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
            send(*termination_message);

        }
        // TODO: It would be better to wait for response from server confirming that it was terminated
        initialized = false;
    }

    void flag_for_deletion() override {
        status = Status::deleted;
        connector.reset();  // delete socket to allow reallocation of ports
    }

    std::string type() override {
        return "server";
    }

    int get_send_port() override {
        return connector->get_send_port();
    }

    int get_recv_port() override {
        return connector->get_recv_port();
    }

    const std::string& get_initialization_address() override {
        return address;
    }


private:
    std::unique_ptr<Connector> connector;
};


// ============================================================================================================


// TODO: Thread and Remote are more or less identical apart from their send and receive. Refactor at some point.
class Thread : public BaseWithParent {
public:
    Thread(const std::string& name
           , const std::string& status_address
           , std::unique_ptr<Connector> connector
           , const std::string parent_name)
            : BaseWithParent(name, "/" + name, status_address, parent_name)
              , connector(std::move(connector)) {}


    bool send(OscSendMessage& msg) override {
        if (status == Status::ready) {
            connector->send(msg);
            return true;
        }

        return false;
    }

    std::vector<c74::min::atoms> receive(const std::string& address) override {
        return connector->receive(address);
    }


    void flag_for_deletion() override {
        status = Status::deleted;
        connector.reset();  // delete socket to allow reallocation of ports
    }

    std::string type() override {
        return "thread";
    }

    int get_send_port() override {
        return connector->get_send_port();
    }

    int get_recv_port() override {
        return connector->get_recv_port();
    }


private:
    std::unique_ptr<Connector> connector;


};

// ============================================================================================================

class Remote : public BaseWithParent {
public:
    Remote(const std::string& name
           , const std::string& status_address
           , const std::string& parent_name)
            : BaseWithParent(name
                             , address_from_full_name(name)
                             , status_address
                             , parent_name) {}


    bool send(OscSendMessage& msg) override {
        if (status == Status::ready) {
            return parent->send(msg);
        }
        return false;
    }

    std::vector<c74::min::atoms> receive(const std::string& address) override {
        if (parent) {
            return parent->receive(address);
        }
        return {};
    }


    void flag_for_deletion() override {
        status = Status::deleted;
    }

    std::string type() override {
        return "remote";
    }

    int get_send_port() override {
        if (parent) {
            return parent->get_send_port();
        } else {
            return -1;
        }
    }

    int get_recv_port() override {
        if (parent) {
            return parent->get_recv_port();
        } else {
            return -1;
        }
    }

private:
    const std::string base_name;

};

#endif //PYOSC_PYOSC_OBJECTS_H
