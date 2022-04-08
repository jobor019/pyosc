#ifndef PYOSC_MAX_BASE_H
#define PYOSC_MAX_BASE_H

#include <regex>

#include "c74_min_api.h"
#include "../src/pyosc_objects.h"
#include "../src/pyosc_base.h"
#include "../src/message_queue.h"
#include "../src/pyosc_manager.h"

using namespace c74::min;


class maxbase : public object<maxbase> {
public:
    inlet<> input{this, "(anything) messages to process"};
    outlet<> main_out{this, "(anything) output from server"};
    outlet<> status_out{this, "(int) object status code"};
    outlet<> print_out{this, "(anything) log messages from server or from this object"};
    outlet<> dump_out{this, "(dumpout)"};


    message<> initialize{this, "initialize", "initialize the remote object", MIN_FUNCTION {

        if (initialization_message) {
            cwarn << "initialization message already queued, overwriting current" << endl;
            initialization_message = std::nullopt;
        }

        try_initialize(args);

        return {};
    }};


    message<> send{this, "send", "send messages to the remote object", MIN_FUNCTION {
        std::unique_ptr<OscSendMessage> msg;
        try {
            // TODO: Fail on error should be settable
            msg = AtomOscParser::atoms2send(communication_object->get_address(), args, &cwarn, false);
        } catch (std::runtime_error& e) {
            cerr << e.what() << endl;
            return {};
        }

        if (status == Status::ready) {
            try {
                communication_object->send(*msg);
            } catch (osc::MalformedMessageException& e) {
                cerr << e.what() << endl;
                return {};
            }
        } else {
            queue.add(std::move(msg));
            dump_out.send("queued " + std::to_string(queue.size()));
        }
        return {};
    }};


    message<> terminate{this, "terminate", "send termination message to the remote object", MIN_FUNCTION {
        // TODO: Handle this with boolean check instead
        communication_object->terminate();
        dump_out.send("terminated");
        return {};
    }};


    message<> terminationmessage{this
                                 , "terminationmessage"
                                 , "Message to send to remote upon termination and/or deletion"
                                 , MIN_FUNCTION {

                atoms out{"terminationmessage"};
                if (args.empty()) {
                    // Get
                    if (termination_message) {
                        out.insert(out.end(), (*termination_message).begin(), (*termination_message).end());
                        dump_out.send(out);
                        return {};

                    } else {
                        dump_out.send(out);
                        return {};
                    }


                } else if (communication_object && communication_object->is_initialized()) {
                    // Invalid: set while already initialized
                    cerr << "cannot set initialization message once object has been initialized" << endl;
                    dump_out.send({"terminationmessage", -1});
                    return {};

                } else {
                    // Set
                    termination_message = std::make_optional(args);
                    out.insert(out.end(), args.begin(), args.end());
                    dump_out.send(out);
                    return {};
                }
            }
    };


    message<> clear{this, "clear", "remove any non-sent message in the queue (if object isn't initialized)"
                    , MIN_FUNCTION {
                auto num_messages = queue.size();
                queue.clear();

                dump_out.send("queued " + std::to_string(queue.size()));
                print_out.send("cleared " + std::to_string(num_messages) + " messages");

                if (initialization_message) {
                    initialization_message = std::nullopt;
                    dump_out.send({"initialize", 0});
                    print_out.send("cleared initialization message");
                }

                return {};
            }};


    message<> name{this, "name", "get name of object (fourth outlet)"
                   , MIN_FUNCTION {
                dump_out.send({"name", communication_object->get_name()});
                return {};
            }};

    message<> sendport{this, "sendport", "get send port (fourth outlet)"
                       , MIN_FUNCTION {
                dump_out.send({"sendport", communication_object->get_send_port()});
                return {};
            }};

    message<> recvport{this, "recvport", "get receive port (fourth outlet)"
                       , MIN_FUNCTION {
                dump_out.send({"recvport", communication_object->get_recv_port()});
                return {};
            }};

    message<> type{this, "type", "get type of object (fourth outlet)"
                   , MIN_FUNCTION {
                dump_out.send({"type", communication_object->type()});
                return {};
            }};


    timer<> receive_poll{this, MIN_FUNCTION {
        receive_poll.delay(pollinterval.get());
        auto messages = communication_object->receive(communication_object->get_address());
        for (auto& msg: messages) {
            try {
                main_out.send(msg);
            } catch (std::runtime_error& e) {
                cerr << e.what() << endl;
            }
        }
        return {};
    }
    };


protected:
    static PortSpec parse_ports(const atoms& args, int send_idx = 2, int recv_idx = 3, int nargs = 4) {
        if (args.size() >= nargs) {
            if (args[send_idx].type() == message_type::int_argument
                && args[recv_idx].type() == message_type::int_argument) {
                return PortSpec{static_cast<int>(args[send_idx])
                                , static_cast<int>(args[recv_idx])};

            } else {
                throw std::runtime_error("send and receive ports must be integer values");
            }

        } else if (args.size() == nargs - 1) {
            // invalid
            throw std::runtime_error("both sendport and recvport must be specified "
                                     "(or leave both unspecified for auto assignment)");
        } else {
            throw std::runtime_error("not enough arguments provided");
        }
    }

    static std::string parse_ip(const atoms& args, int ip_idx = 1) {
        if (args.size() > ip_idx) {
            if (args[ip_idx].type() == message_type::symbol_argument) {
                auto ip_str = static_cast<std::string>(args[ip_idx]);
                if (ip_str == "localhost") {
                    return "127.0.0.1";
                } else {
                    return ip_str;
                }
            } else {
                throw std::runtime_error("ip address must be a single symbol");
            }
        }

        return std::string{"127.0.0.1"};
    }

    // Note: does not check if address is a valid ip, only whether it's formatted in a manner similar to an ip4 address
    static bool is_ip_like(const atoms& args, int ip_idx = 1) {
        if (args.size() > ip_idx && args[ip_idx].type() == message_type::symbol_argument) {
            std::regex r("([0-9.]+)|localhost");
            auto ip_candidate = static_cast<std::string>(args[ip_idx]);
            return std::regex_match(ip_candidate, r);

        } else {
            return false;
        }
    }


    static std::string parse_name(const atoms& args, int name_idx = 0, bool is_parent = false) {
        if (args.size() > name_idx) {
            // name, ...
            if (args[name_idx].type() == message_type::symbol_argument) {
                return static_cast<std::string>(args[name_idx]);
            } else { // error: message depends on what type of name is parsed
                if (is_parent) {
                    throw std::runtime_error("parent name must be a single symbol");
                } else {
                    throw std::runtime_error("name must be a single symbol");
                }
            }

        } else { // error: message depends on what type of name is parsed
            if (is_parent) {
                throw std::runtime_error("parent name must be provided");
            } else {
                throw std::runtime_error("name must be provided");
            }
        }
    }


    static bool is_remote_str(const atoms& args, int remote_idx = 2) {
        return args.size() > remote_idx
               && args[remote_idx].type() == message_type::symbol_argument
               && static_cast<std::string>(args[remote_idx]) == "remote";
    }

    void try_initialize(const atoms& init_msg) {
        try {
            // both of these throw std::runtime_error on empty messages
            //               return nullptr if explicitly specified as empty
            //               return an osc message otherwise
            auto init_osc_msg = parse_initialization_message(init_msg);
            auto termination_osc_msg = parse_termination_message(termination_message);

            bool res = communication_object->initialize(std::move(init_osc_msg), std::move(termination_osc_msg));

            if (!res) {
                // Initialization failed because object isn't ready: queue init message
                initialization_message = init_msg;
            } else {
                // Initialization succeeded: clear message if stored
                initialization_message = std::nullopt;
            }

            dump_out.send({"initialize ", res});

        } catch (std::runtime_error& e) {
            // Object already initialized
            cerr << e.what() << endl;

            dump_out.send({"initialize", -1});
        }
    }

    /**
     * @throws std::runtime_error if object does not have a parent or if passing an empty message.
     * @returns nullptr if message is explicitly specified as empty, otherwise string
     */
    std::unique_ptr<OscSendMessage> parse_initialization_message(const atoms& message, bool is_initialization = true) {
        if (!communication_object) {
            // Should technically never happen
            throw std::runtime_error("object not yet created");
        }

        if (message.empty() ||
            (message.size() == 1
             && message[0].type() == message_type::symbol_argument
             && static_cast<std::string>(message[0]).empty())) {
            // empty message or single message consisting of empty string
            std::stringstream err;
            err << "no " << (is_initialization ? "initialization" : "termination") << " message provided";
            throw std::runtime_error(err.str());

        } else if (message.size() == 1 && message[0].type() == message_type::symbol_argument
                   && static_cast<std::string>(message[0]) == "null") {
            // No initialization/termination message
            return nullptr;

        } else {
            // parse message. throws std::runtime_error if communication object does not have a termination_address yet
            return AtomOscParser::atoms2send(communication_object->get_initialization_address()
                                             , message, &cwarn, false);
        }

    }

    std::unique_ptr<OscSendMessage> parse_termination_message(std::optional<atoms> message) {
        if (message) {
            return parse_initialization_message(*message, false);
        } else {
            throw std::runtime_error("no termination message provided");
        }
    }


protected:
    std::shared_ptr<BaseOscObject> communication_object;
    MessageQueue queue;
    std::optional<c74::min::atoms> initialization_message;
    std::optional<c74::min::atoms> termination_message;
    Status status = Status::offline;

    void process_queue_unsafe() {
        while (!queue.empty()) {
            auto msg = queue.pop();
            // TODO: Handle this more safely by checking status of each send
            bool res = communication_object->send(*msg);
            dump_out.send("queued ", queue.size());
        }
    }


public:
    attribute<int> pollinterval{this, "pollinterval", 1
                                , description{"Interval for polling the server for new messages"}
                                , setter{
                    MIN_FUNCTION {
                        if (args.empty()) {
                            cout << "must provide a value for pollinterval" << endl;
                            dump_out.send({"pollinterval", pollinterval.get()});
                            return {pollinterval.get()};

                        } else if (args.size() == 1
                                   && args[0].type() == message_type::int_argument
                                   && static_cast<int>(args[0]) >= 0) {
                            dump_out.send({"pollinterval", args[0]});
                            return args;

                        } else {
                            cout << "pollinterval must be a single integer greater than or equal to 0" << endl;
                            dump_out.send({"pollinterval", pollinterval.get()});
                            return {pollinterval.get()};
                        }
                    }
            }
    };

    attribute<int> statusinterval{this, "statusinterval", 500
                                  , description{"Interval for polling the status of the server"}
                                  , setter{
                    MIN_FUNCTION {
                        if (args.empty()) {
                            cout << "must provide a value for statusintjerval" << endl;
                            dump_out.send({"statusinterval", statusinterval.get()});
                            return {statusinterval.get()};

                        } else if (args.size() == 1
                                   && args[0].type() == message_type::int_argument
                                   && static_cast<int>(args[0]) >= 0) {
                            dump_out.send({"statusinterval", args[0]});
                            return args;

                        } else {
                            cout << "statusinterval must be a single integer greater than or equal to 0" << endl;
                            dump_out.send({"statusinterval", statusinterval.get()});
                            return {statusinterval.get()};
                        }
                    }
            }
    };

    attribute<symbol> address{this, "address", "/server"
                              , description{"OSC address for sending and receiving messages from/to this object"}
                              , setter{
                    MIN_FUNCTION {

                        if (args.empty()) {
                            dump_out.send({"address", communication_object->get_address()});
                            return {};
                        }

                        // TODO: Fix so that it's settable after initialization
                        if (communication_object) {
                            cout << "cannot set after initialization" << endl;
                            return {address.get()};
                        }

                        if (status != Status::offline) {
                            cout << "cannot set address after initialization" << endl;
                            dump_out.send({"address", address.get()});
                            return {address.get()};

                        } else if (args.empty()) {
                            cout << "must provide a value for address" << endl;
                            dump_out.send({"address", address.get()});
                            return {address.get()};

                        } else if (args.size() == 1 && args[0].type() == message_type::symbol_argument) {
                            auto s = static_cast<std::string>(args[0]);

                            if (s.find('/') == 0) {
                                dump_out.send({"address", args[0]});
                                return {args};

                            } else {
                                cout << "OSC address must begin with '/'" << endl;
                                dump_out.send({"address", address.get()});
                                return {address.get()};
                            }


                        } else {
                            cout << "address must be a single symbol" << endl;
                            dump_out.send({"address", address.get()});
                            return {address.get()};
                        }
                    }
            }};


    attribute<symbol> statusaddress{this, "statusaddress", "/status"
                                    , description{
                    "OSC address for sending and receiving status messages from/to this object"}
                                    , setter{
                    MIN_FUNCTION {

                        if (args.empty()) {
                            dump_out.send({"statusaddress", communication_object->get_status_address()});
                            return {};
                        }

                        // TODO: Fix so that it's settable after initialization
                        if (communication_object) {
                            cout << "cannot set after initialization" << endl;
                            return {statusaddress.get()};
                        }

                        if (status != Status::offline) {
                            cout << "cannot set statusaddress after initialization" << endl;
                            dump_out.send({"statusaddress", statusaddress.get()});
                            return {statusaddress.get()};

                        } else if (args.empty()) {
                            cout << "must provide a value for statusaddress" << endl;
                            dump_out.send({"statusaddress", statusaddress.get()});
                            return {statusaddress.get()};

                        } else if (args.size() == 1 && args[0].type() == message_type::symbol_argument) {
                            auto s = static_cast<std::string>(args[0]);

                            if (s.find('/') == 0) {
                                dump_out.send({"statusaddress", args[0]});
                                return {args};

                            } else {
                                cout << "OSC statusaddress must begin with '/'" << endl;
                                dump_out.send({"statusaddress", statusaddress.get()});
                                return {statusaddress.get()};
                            }


                        } else {
                            cout << "statusaddress must be a single symbol" << endl;
                            dump_out.send({"statusaddress", statusaddress.get()});
                            return {statusaddress.get()};
                        }
                    }
            }};


};

#endif //PYOSC_MAX_BASE_H
