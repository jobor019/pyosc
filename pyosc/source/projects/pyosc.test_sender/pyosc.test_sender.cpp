/// @file
///	@ingroup 	minexamples
///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#include "c74_min.h"
#include "../shared/agent_object.h"
#include "../shared/juce_osc_receiver.h"

using namespace c74::min;


class test_sender : public object<test_sender> {
public:

    outlet<> output{this, "(anything) output from osc port"};

    explicit test_sender(const atoms& args = {}) {
        if (args.size() == 1 && args[0].type() == message_type::int_argument) {
            sender.connect("127.0.0.1", static_cast<int>(args[0]));
            cout << "initialized" << endl;
        } else {
            error("Invalid args");
        }

    }

    message<> bang{this, "bang", "Send.", MIN_FUNCTION {
        bool res = sender.send(juce::OSCMessage("/hehe"));
        if (!res) {
            cout << "fail send" << endl;
        }
        return {};
    }
    };


private:
    juce::OSCSender sender;
};


MIN_EXTERNAL(test_sender);
