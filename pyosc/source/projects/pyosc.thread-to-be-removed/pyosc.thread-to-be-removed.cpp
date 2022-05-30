/// @file
///	@ingroup 	minexamples
///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#include "c74_min.h"
#include "../src/max_base.h"

using namespace c74::min;

// TODO: This entire class should be removed
class thread : public maxbase {
public:
    // TODO: Description, tags, author, related
    MIN_DESCRIPTION{"Communicate with a server architecture over OSC."};
    MIN_TAGS{"utilities"};
    MIN_AUTHOR{"Joakim Borg"};
    MIN_RELATED{"pyosc.argparse, pyosc.pyosc, udpsend, udpreceive"};

    explicit thread(const atoms& args = {}) {
        try {
            std::string name = parse_name(args, 0, 1, false);
            std::string parent_name = parse_name(args, 1, 2, true);
            std::string ip = parse_ip(args, 2, 3);
            std::optional<PortSpec> port_spec = parse_ports(args, 3, 4, 5);


            communication_object = PyOscManager::get_instance().new_thread(name
                                                                           , statusaddress.get()
                                                                           , ip
                                                                           , port_spec
                                                                           , parent_name);

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

        if (new_status == Status::parent_obj_missing) {
            auto parent = PyOscManager::get_instance().get_object(communication_object->get_name());
            if (parent) {
                std::dynamic_pointer_cast<Thread>(communication_object)->set_parent(parent);
            }
        }

        handle_status(new_status);
        return {};
    }};

protected:


};


MIN_EXTERNAL(thread);
