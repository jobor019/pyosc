/// @file
///	@ingroup 	minexamples
///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.


#include "c74_min.h"

using namespace c74::min;

// TODO: This entire class should be removed
class argparse : public object<argparse>  {
public:
    // TODO: Description, tags, author, related
    MIN_DESCRIPTION{"Parse arguments with correct format for pyosc.pyosc."};
    MIN_TAGS{"utilities"};
    MIN_AUTHOR{"Joakim Borg"};
    MIN_RELATED{"pyosc.pyosc"};

    outlet<> out{this, "(list) formatted message"};


    explicit argparse(const atoms& args = {}) {
        if (args.empty()) {
            error("at least one argument must be provided");
        }

        dynamic_inlets.emplace_back(std::make_unique<inlet<> >(this, "(bang) output message"));

        for (auto& e: args) {
            if (e.type() != message_type::symbol_argument) {
                error("only symbol arguments are supported");
            }

            dynamic_inlets.emplace_back(std::make_unique<inlet<> >(this, static_cast<std::string>(e)));
            keywords.emplace_back(e);
            values.emplace_back(atoms{});
        }
    }

    c74::min::function store_message = MIN_FUNCTION {
        values.at(inlet - 1) = args;    // first inlet is bang inlet
        return {};
    };

    message<> bang { this, "bang", "(in first inlet) output message", MIN_FUNCTION {
        if (inlet != 0) {
            cerr << "message type not supported in this inlet" << endl;
            return {};
        }

        atoms msg_out{};
        for (int i = 0; i < keywords.size(); ++i) {

            if (values.at(i).empty()) {
                cerr << "no value provided for '" << keywords.at(i) << "'" << endl;
                return {};
            }

            msg_out.emplace_back(keywords.at(i));
            msg_out.insert(std::end(msg_out), std::begin(values.at(i)), std::end(values.at(i)));
        }
        out.send(msg_out);
        return {};
    }};


    // handle str and list starting w/ string
    message<> anything { this, "anything", "set value of argument", store_message};
    // handle list starting w/ float/int
    message<> list { this, "list", "set value of argument", store_message};
    // handle list of length 1 (float/int)
    message<> number { this, "number", "set value of argument", store_message};


private:
    std::vector<atom> keywords;
    std::vector<std::vector<atom> > values;
    std::vector<std::unique_ptr<inlet<> > > dynamic_inlets;

};


MIN_EXTERNAL(argparse);

