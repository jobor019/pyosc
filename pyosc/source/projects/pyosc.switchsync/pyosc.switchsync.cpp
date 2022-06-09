/// @file
///	@ingroup 	minexamples
///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.


#include <map>
#include "c74_min.h"

using namespace c74::min;

class syncswitch : public object<syncswitch> {
public:
    MIN_DESCRIPTION{"Synchronize mutually exclusive configurations of messages"};
    MIN_TAGS{"utilities"};
    MIN_AUTHOR{"Joakim Borg"};
    MIN_RELATED{"pyosc.pyosc, pyosc.argparse"};

    inlet<> ctrl_inlet{this, "(int) non-zero open gate by index, 0 close all"};

    outlet<> selector_out{this, "(int) selector chosen"};
    outlet<> messages_out{this, "(list) messages out"};
    outlet<> dump_out{this, "(dumpout)"};


    explicit syncswitch(const atoms& args = {}) {
        if (args.size() != 1 || args[0].type() != message_type::int_argument) {
            error("bad argument for message \"syncswitch\"");
        }

        auto num_inlets = static_cast<int>(args[0]);
        for (int i = 0; i < num_inlets; ++i) {
            int max_index = i + 1;
            dynamic_inlets.emplace_back(std::make_unique<inlet<> >(this, "(list) messages for selector "
                                                                         + std::to_string(max_index)));
            stored_messages.emplace_back(std::map<std::string, atoms>());
        }
    }

    c74::min::function process_incoming_message = MIN_FUNCTION {
        if (inlet == 0) {
            cerr << "doesn't understand message " << args[0] << endl;
            return {};
        }

        if (args.size() < 2) {
            cerr << "list must contain at least two values" << endl;
            return {};
        }

        if (args[0].type() != message_type::symbol_argument) {
            // Should never happen: won't be treated as `anything` then
            cerr << "first argument must be a string" << endl;
            return {};
        }

        // inlet index will always be valid since it's not user-controlled
        int max_inlet_index = inlet;                // actual max inlet index
        int stored_messages_index = inlet - 1;      // corresponding data index

        // already checked type and size
        auto key = static_cast<std::string>(args.at(0));

        stored_messages.at(stored_messages_index).insert_or_assign(key, args);
        dump_out.send("stored", max_inlet_index, key);

        if (max_inlet_index == open_gate) {
            messages_out.send(args);
        }

        return {};
    };

    message<> bang{this, "bang", "(in first inlet) output current state", MIN_FUNCTION {

        if (inlet != 0) {
            cerr << "message \"bang\" not supported in this inlet" << endl;
            return {};
        }

        dump_out.send("open", open_gate);
        output_state();
        return {};
    }};

    message<> clear {this, "clear", "(in first inlet) clears the state of the object and closes all gates",
                    MIN_FUNCTION {
        if (inlet != 0) {
            cerr << "message \"clear\" not supported in this inlet" << endl;
        }

        for (auto& e : stored_messages) {
            e.clear();
        }

        open_gate = 0;
        dump_out.send("open", open_gate);
        dump_out.send("clear");
        return {};
    }};


    // handle str and lists specifically starting w/ string
    message<> anything { this, "anything", "set value of argument", process_incoming_message};

    // handle list of length 1 (float/int)
    message<> number {this, "number", "set value of argument", MIN_FUNCTION {
        if (inlet != 0) {
            cerr << "message \"int\" is only supported in first inlet" << endl;
            return {};
        }

        // 1-indexed: first dynamic inlet is index 1
        auto new_open = static_cast<int>(args[0]);

        if (new_open > dynamic_inlets.size() || new_open < 0) {
            cerr << "inlet " << new_open << " does not exist" << endl;
            return {};
        }

        dump_out.send("open", open_gate);
        if (new_open > 0 && new_open != open_gate) {
            open_gate = new_open;
            output_state();
        } else {
            open_gate = new_open;

        }

        return {};
    }};


private:
    std::vector<std::unique_ptr<inlet<> > > dynamic_inlets;
    std::vector<std::map<std::string, atoms> > stored_messages;
    int open_gate = 0;

    void output_state() {
        selector_out.send(open_gate);    // selector first

        if (open_gate > 0 && open_gate <= dynamic_inlets.size()) {
            int stored_messages_index = open_gate - 1;
            for (auto& e: stored_messages.at(stored_messages_index)) {
                messages_out.send(e.second);
            }
        }
    }


};


MIN_EXTERNAL(syncswitch);

