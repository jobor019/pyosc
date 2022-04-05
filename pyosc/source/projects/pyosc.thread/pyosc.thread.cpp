/// @file
///	@ingroup 	minexamples
///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#include "c74_min.h"
#include "../src/max_base.h"

using namespace c74::min;


class thread : public maxbase {
public:
    // TODO: Description, tags, author, related
    MIN_DESCRIPTION{"Communicate with a server architecture over OSC."};
    MIN_TAGS{"utilities"};
    MIN_AUTHOR{"Joakim Borg"};
    MIN_RELATED{"pyosc.server, pyosc.remote, udpsend, udpreceive"};

    explicit thread(const atoms& args = {}) {
        try {
            std::optional<PortSpec> port_spec = parse_ports(args);
            std::string ip = parse_ip(args);
            std::string name = parse_name(args);

            communication_object = PyOscManager::get_instance().new_thread(name, statusaddress.get(), ip, port_spec);

            status_poll.delay(0.0);
            receive_poll.delay(0.0);
        } catch (std::runtime_error& e) {
            error(e.what());
        }
    }


    ~thread() override {
        PyOscManager::get_instance().remove_object(communication_object->get_name());
    }


    timer<> status_poll{this, MIN_FUNCTION {
        communication_object->update_status();
        auto new_status = communication_object->get_status();

        status_out.send(static_cast<int>(new_status));

        if (new_status != status) {
            status = new_status;
        }
        // TODO
    }};

protected:

    attribute<symbol> terminationmessage{this, "terminationmessage", ""
                                    , description{
                    "Message to send to parent upon deleting this object in max"}
                                    , setter{
                    MIN_FUNCTION {

                        if (args.empty()) {
                            dump_out.send({"terminationmessage", communication_object->get_status_address()});
                            return {};
                        }

                        if (communication_object) {
                            try {
                                communication_object->set
                            }
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


MIN_EXTERNAL(thread);
