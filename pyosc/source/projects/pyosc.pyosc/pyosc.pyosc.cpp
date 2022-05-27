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


class pyosc : public maxbase {
public:
    // TODO: Description, tags, author, related
    MIN_DESCRIPTION{"Communicate with a server architecture over OSC."};
    MIN_TAGS{"utilities"};
    MIN_AUTHOR{"Joakim Borg"};
    MIN_RELATED{"udpsend, udpreceive"};


    /**
     * Initialization Messages
     * Server:                  <name>
     *                          <name> <ip>
     *                          <name> <ip> <sendport> <recvport>
     *
     * Thread (Agent):          <name> <parent>
     *                          <name> <parent> <ip>
     *                          <name> <parent> <ip> <sendport> <recvport>
     *
     * Remote (Subcomponent):   <name> <parent> "remote" (literal keyword)
     */
    explicit pyosc(const atoms& args = {}) {
        try {

            std::optional<std::string> parent_name;
            std::optional<std::string> ip;
            std::optional<PortSpec> port_spec;
            bool is_remote;


            // First argument should always be name
            std::string name = parse_name(args, 0, false);


            if (args.size() == 2) {
                // Thread with auto port/ip or Server with manual ip
                if (is_ip_like(args, 1)) {
                    // Server
                    ip = std::make_optional(parse_ip(args, 1));
                } else {
                    // Thread
                    parent_name = std::make_optional(parse_name(args, 1, true));
                }

            } else if (args.size() == 3) {
                // Remote or Thread with manual ip
                parent_name = std::make_optional(parse_name(args, 1, true));

                if (is_remote_str(args, 2)) {
                    // Remote
                    is_remote = true;

                } else {
                    // Thread
                    ip = std::make_optional(parse_ip(args, 2));
                }


            } else if (args.size() == 4) {
                // Server with manual ip and port
                ip = parse_ip(args, 1);
                port_spec = std::make_optional(parse_ports(args, 2, 3, 4));

            } else if (args.size() == 5) {
                // Thread with manual ip and ports
                parent_name = std::make_optional(parse_name(args, 1, true));
                ip = parse_ip(args, 2);
                port_spec = std::make_optional(parse_ports(args, 3, 4, 5));
            }

            communication_object = PyOscManager::get_instance().new_object(name
                                                                           , statusaddress.get()
                                                                           , parent_name
                                                                           , ip
                                                                           , port_spec
                                                                           , is_remote);


            status_poll.delay(0.0);
            receive_poll.delay(0.0);

        } catch (std::runtime_error& e) {
            error(e.what());
        }
    }

    ~pyosc() override {
        if (communication_object) {
            communication_object->terminate();
            PyOscManager::get_instance().remove_object(communication_object->get_name());
        }
    }


    timer<> status_poll{this, MIN_FUNCTION {

        communication_object->update_status();
        auto new_status = communication_object->get_status();
        status = new_status;

        status_out.send(static_cast<int>(status));


        if (status == Status::parent_obj_missing) {
            // Should only happens for Thread and Remote, but checking just to be sure
            auto obj = std::dynamic_pointer_cast<BaseWithParent>(communication_object);

            if (obj) {
                auto parent = PyOscManager::get_instance().get_object(obj->get_parent_name());
                if (parent) {
                    obj->set_parent(parent);
                }
            }

        } else if (status == Status::uninitialized && initialization_message_content) {
            try_initialize(*initialization_message_content);

        } else if (status == Status::ready && !queue.empty()) {
            process_queue_unsafe();
        }
        // all other cases: wait for new status
        status_poll.delay(statusinterval.get());
        return {};
    }
    };

};


MIN_EXTERNAL(pyosc);

