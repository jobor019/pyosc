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

// TODO: This entire class should be removed
class server : public maxbase {
public:
    // TODO: Description, tags, author, related
    MIN_DESCRIPTION{"Communicate with a server architecture over OSC."};
    MIN_TAGS{"utilities"};
    MIN_AUTHOR{"Joakim Borg"};
    MIN_RELATED{"pyosc.thread-to-be-removed, pyosc.pyosc, udpsend, udpreceive"};


    explicit server(const atoms& args = {}) {
        try {
            std::optional<PortSpec> port_spec = parse_ports(args);
            std::string ip = parse_ip(args);
            std::string name = parse_name(args);

            communication_object = PyOscManager::get_instance().new_server(name
                                                                           , statusaddress.get()
                                                                           , ip
                                                                           , port_spec);


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

        handle_status(new_status);

        status_poll.delay(statusinterval.get());
        return {};
    }
    };

};


MIN_EXTERNAL(server);
