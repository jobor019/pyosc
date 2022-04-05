/// @file
///	@ingroup 	minexamples
///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.


#include "c74_min.h"
#include "../src/pyosc_objects.h"
#include "../src/message_queue.h"
#include "../src/pyosc_manager.h"
#include "../src/max_base.h"

using namespace c74::min;


class server : public maxbase {
public:
    // TODO: Description, tags, author, related
    MIN_DESCRIPTION{"Communicate with a server architecture over OSC."};
    MIN_TAGS{"utilities"};
    MIN_AUTHOR{"Joakim Borg"};
    MIN_RELATED{"pyosc.thread, pyosc.remote, udpsend, udpreceive"};


    explicit server(const atoms& args = {}) {
        try {
            std::optional<PortSpec> port_spec = parse_ports(args);
            std::string ip = parse_ip(args);
            std::string name = parse_name(args);

            communication_object = PyOscManager::get_instance().new_server(name
                                                                           , statusaddress.get()
                                                                           , ip
                                                                           , port_spec);

            // set value of attributes that were not passed in ctor and MAY have been set before ctor
            auto termination_message = static_cast<std::string>(terminationmessage.get());
            if (!termination_message.empty()) {
                communication_object->set_termination_message({termination_message});
            }

            status_poll.delay(0.0);
            receive_poll.delay(0.0);

        } catch (std::runtime_error& e) {
            error(e.what());
        }
    }

    ~server() override {
        PyOscManager::get_instance().remove_object(communication_object->get_name());
    }


    timer<> status_poll{this, MIN_FUNCTION {
        communication_object->update_status();
        auto new_status = communication_object->get_status();

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

};


MIN_EXTERNAL(server);

