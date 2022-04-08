/// @file
///	@ingroup 	minexamples
///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#include "c74_min.h"
#include "../shared/agent_object.h"
#include "../shared/juce_osc_receiver.h"

using namespace c74::min;

// TODO: This entire class should be removed
class test_receiver : public object<test_receiver> {
public:

    outlet<> output{this, "(anything) output from osc port"};

    explicit test_receiver(const atoms& args = {}) {
        if (args.size() == 1 && args[0].type() == message_type::int_argument) {
            recv.connect(static_cast<int>(args[0]));
            metro.delay(0.0);
        } else {
            error("Invalid args");
        }

    }

    timer<> metro{this, MIN_FUNCTION {
        auto m = recv.readMessage();
        while (m) {
            output.send("bang");
            m = recv.readMessage();
        }
        metro.delay(0.1);
        return {};
    }
    };


private:
    PyOscReceiver recv;
};


MIN_EXTERNAL(test_receiver);
