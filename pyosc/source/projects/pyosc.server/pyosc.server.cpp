/// @file
///	@ingroup 	minexamples
///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#include "c74_min.h"
#include "../src/pyosc_base.h"
#include "../src/message_queue.h"
#include "pyosc_manager.h"
#include "../shared/atom_osc_parser.h"

using namespace c74::min;


class server : public object<server> {
public:
    // TODO: Description, tags, author, related
    MIN_DESCRIPTION{"Communicate with a server architecture over OSC."};
    MIN_TAGS{"utilities"};
    MIN_AUTHOR{"Joakim Borg"};
    MIN_RELATED{"pyosc.thread, pyosc.remote, udpsend, udpreceive"};

    inlet<> input{this, "(anything) messages to process"};
    outlet<> main_out{this, "(anything) output from server"};
    outlet<> status_out{this, "(int) object status code"};
    outlet<> print_out{this, "(anything) log messages from server or from this object"};
    outlet<> dump_out{this, "(dumpout)"};

    explicit server(const atoms& args = {}) {
        try {
            std::optional<PortSpec> port_spec;
            if (args.size() == 4) {
                // name, ip, sendport, recvport
                if (args[2].type() == message_type::int_argument && args[3].type() == message_type::int_argument) {
                    port_spec = std::make_optional(PortSpec{static_cast<int>(args[2]), static_cast<int>(args[3])});

                } else {
                    throw std::runtime_error("send and receive ports must be integer values");
                }

            } else if (args.size() == 3) {
                // invalid
                throw std::runtime_error("both sendport and recvport must be specified "
                                         "(or leave both unspecified for auto assignment)");
            } else {
                port_spec = std::nullopt;
            }

            std::string ip;
            if (args.size() >= 2) {
                // name, ip, ...
                if (args[1].type() == message_type::symbol_argument) {
                    ip = static_cast<std::string>(args[1]);
                } else {
                    throw std::runtime_error("ip address must be a single symbol");
                }
            }

            std::string name;
            if (!args.empty()) {
                // name, ...
                if (args[0].type() == message_type::symbol_argument) {
                    name = static_cast<std::string>(args[0]);
                } else {
                    throw std::runtime_error("name must be a single symbol");
                }

            } else {
                throw std::runtime_error("a name must be provided");
            }

            communication_object = PyOscManager::get_instance().new_server(name
                                                                           , statusaddress.get()
                                                                           , terminationmessage.get()
                                                                           , ip
                                                                           , port_spec);

//            sendport.get() = communication_object->get_send_port();
//            recvport.get() = communicatiodn_object->get_recv_port();


            status_poll.delay(0.0);
            receive_poll.delay(0.0);

        } catch (std::runtime_error& e) {
            error(e.what());
        }
    }

    ~server() override {
        PyOscManager::get_instance().remove_object(communication_object->get_name());
    }

    message<> initialize{this, "initialize", "initialize the remote object", MIN_FUNCTION {
        std::string init_msg;

        if (args.empty()) {
            init_msg = "";
        } else if (args.size() == 1 && args[0].type() == message_type::symbol_argument) {
            init_msg = static_cast<std::string>(args[0]);
        } else {
            cerr << "message 'initialize' takes a single symbol as input" << endl;
            return {};
        }

        if (initialization_message) {
            cwarn << "initialization message already queued, overwriting current" << endl;
        }


        try {
            bool res = communication_object->initialize(init_msg);
            if (!res) {
                // Initialization failed because object isn't ready: queue init message
                initialization_message = init_msg;
            }
            dump_out.send("initialize " + std::to_string(res));


        } catch (std::runtime_error& e) {
            // Object already initialized
            cerr << e.what() << endl;
            dump_out.send("initialize " + std::to_string(-1));
            return {};
        }

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

    message<> clear{this, "clear", "remove any non-sent message in the queue (if object isn't initialized)"
                    , MIN_FUNCTION {
                auto num_messages = queue.size();
                queue.clear();
                dump_out.send("queued " + std::to_string(queue.size()));
                print_out.send("cleared " + std::to_string(num_messages) + " messages");

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

    timer<> status_poll{this, MIN_FUNCTION {
        communication_object->update_status();
        auto new_status = communication_object->get_status();

        std::cout << new_status << "\n";

        status_out.send(static_cast<int>(new_status));

        if (new_status != status) {
            status = new_status;

            if (status == Status::uninitialized && initialization_message) {
                bool res = communication_object->initialize(*initialization_message);

                if (res) {
                    initialization_message = std::nullopt;
                }

            } else if (status == Status::ready && !queue.empty()) {
                process_queue_unsafe();
            }
            // all other cases: wait for new status
        }


        status_poll.delay(statusinterval.get());
        return {};
    }
    };

    timer<> receive_poll{this, MIN_FUNCTION {
        receive_poll.delay(pollinterval.get());
        auto messages = communication_object->receive(communication_object->get_address());
        for (auto& msg: messages) {
            try {
                // TODO: fail_on_error should be settable
                auto atms = AtomOscParser::recv2atoms(msg, &cwarn, false);
                main_out.send(atms);
            } catch (std::runtime_error& e) {
                cerr << e.what() << endl;
            }
        }
        return {};
    }
    };

protected:
    std::shared_ptr<Server> communication_object;
    MessageQueue queue;
    std::optional<std::string> initialization_message;
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

//    attribute<int> sendport{this, "sendport", -1, description{"udp port for sending messages to the server"}
//                            , setter{
//                    MIN_FUNCTION {
//                        // TODO: Fix this so that it can be set after constructor is called
//                        if (communication_object) {
//                            cout << "cannot set sendport after initialization" << endl;
//                            dump_out.send({"sendport", sendport.get()});
//                            return {sendport.get()};
//                        } else {
//                            dump_out.send({"sendport", sendport.get()});
//                            return {communication_object->get_send_port()};
//                        }
//                        // TODO: (end)
//
//                        if (status != Status::offline) {
//                            cout << "cannot set sendport after initialization" << endl;
//                            dump_out.send({"sendport", sendport.get()});
//                            return {sendport.get()};
//
//                        } else if (args.empty()) {
//                            dump_out.send({"sendport", sendport.get()});
//                            return {sendport.get()};
//
//                        } else if (args.size() == 1 && args[0].type() == message_type::int_argument) {
//                            // Note: no check for valid port range,
//                            //  this will be checked upon initializing the communication object
//                            dump_out.send({"sendport", args[0]});
//                            return {args};
//
//                        } else {
//                            cout << "sendport must be a single integer" << endl;
//                            dump_out.send({"sendport", sendport.get()});
//                            return {sendport.get()};
//                        }
//                    }
//            }};
//
//    attribute<int> recvport{this, "recvport", -1, description{"udp port for receiving messages from the server"}
//                            , setter{
//                    MIN_FUNCTION {
//                        if (status != Status::offline) {
//                            cout << "cannot set recvport after initialization" << endl;
//                            dump_out.send({"recvport", recvport.get()});
//                            return {recvport.get()};
//
//                        } else if (args.empty()) {
//                            dump_out.send({"recvport", recvport.get()});
//                            return {recvport.get()};
//
//                        } else if (args.size() == 1 && args[0].type() == message_type::int_argument) {
//                            // Note: no check for valid port range,
//                            //  this will be checked upon initializing the communication object
//                            dump_out.send({"recvport", args[0]});
//                            return {args};
//
//                        } else {
//                            cout << "recvport must be a single integer" << endl;
//                            dump_out.send({"recvport", recvport.get()});
//                            return {recvport.get()};
//                        }
//                    }
//            }};
//
//
//    attribute<symbol> ip{this, "ip", "127.0.0.1", description{"ip address of the server"}
//                         , setter{
//                    MIN_FUNCTION {
//                        if (status != Status::offline) {
//                            cout << "cannot set ip address after initialization" << endl;
//                            dump_out.send({"ip", ip.get()});
//                            return {ip.get()};
//
//                        } else if (args.empty()) {
//                            cout << "must provide a value for recvport" << endl;
//                            dump_out.send({"ip", ip.get()});
//                            return {ip.get()};
//
//                        } else if (args.size() == 1 && args[0].type() == message_type::symbol_argument) {
//                            // Note: no check for valid ip address,
//                            //  this will be checked upon initializing the communication object
//                            dump_out.send({"ip", args[0]});
//                            return {args};
//
//                        } else {
//                            cout << "ip must be a single symbol" << endl;
//                            dump_out.send({"ip", recvport.get()});
//                            return {ip.get()};
//                        }
//                    }
//            }};

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

    attribute<symbol> terminationmessage{this, "terminationmessage", ""
                                         , description{"Message to send to server upon deletion"}
                                         , setter{
                    MIN_FUNCTION {
                        // TODO: Fix so that it's settable after initialization
                        if (communication_object) {
                            cout << "cannot set after initialization" << endl;
                            return {terminationmessage.get()};
                        }

                        if (status != Status::offline) {
                            // TODO: This should be possible to pass at a later stage
                            cout << "cannot set terminationmessag after initialization" << endl;
                            dump_out.send({"terminationmessage", terminationmessage.get()});
                            return {terminationmessage.get()};
                        } else if (args.empty()) {
                            dump_out.send({"terminationmessage", ""});
                            return {terminationmessage.get()};

                        } else if (args.size() == 1 && args[0].type() == message_type::symbol_argument) {
                            dump_out.send({"terminationmessage", args[0]});
                            return {args};

                        } else {
                            cout << "terminationmessage must be a single symbol" << endl;
                            dump_out.send({"terminationmessage", terminationmessage.get()});
                            return {terminationmessage.get()};
                        }
                    }
            }};


};


MIN_EXTERNAL(server);
