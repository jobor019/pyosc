
#ifndef PYOSC_ATOM_OSC_PARSER_H
#define PYOSC_ATOM_OSC_PARSER_H

#include <iostream>
#include "OscReceivedElements.h"
#include "osc_send_message.h"
#include "c74_min_api.h"


class AtomOscParser {
public:

    static std::unique_ptr<OscSendMessage> atoms2send(const c74::min::atoms& args) {
        if (args.empty()) {
            throw std::runtime_error("an OSC address must be provided");
        }

        // throws std::runtime_error if invalid type or invalid OSC address
        std::string address = parse_address(args[0]);

        auto msg = std::make_unique<OscSendMessage>(address);
        auto it = std::next(args.begin());  // iterate from second element
        while (it != args.end()) {
            if (it->type() == c74::min::message_type::int_argument) {
                msg->add_int(static_cast<int>(*it));

            } else if (it->type() == c74::min::message_type::float_argument) {
                msg->add_double(static_cast<double>(*it));

            } else if (it->type() == c74::min::message_type::symbol_argument) {
                msg->add_string(static_cast<std::string>(*it));

            } else {
                throw std::runtime_error("element of unsupported data type encountered in message");
            }
            ++it;
        }
        return std::move(msg);
    }

    static c74::min::atoms recv2atoms(const osc::ReceivedMessage& msg) {
        c74::min::atoms args{std::string(msg.AddressPattern())};
        auto it = msg.ArgumentsBegin();
        while (it != msg.ArgumentsEnd()) {
            if (it->IsInt32()) {
                args.push_back(it->AsInt32());
            } else if (it->IsInt64()) {
                args.push_back(it->AsInt64());
            } else if (it->IsFloat()) {
                args.push_back(it->AsFloat());
            } else if (it->IsDouble()) {
                args.push_back(it->AsDouble());
            } else if (it->IsString()) {
                std::cout << it->AsString() << "\n";
                args.push_back(it->AsString());
            } else if (it->IsChar()) {
                args.push_back(it->AsChar());
            } else if (it->IsBool()) {
                args.push_back(it->AsBool());
            } else {
                return {std::string("invalid"), std::string(msg.AddressPattern()), msg.ArgumentCount()};
            }
            ++it;
        }
        return args;
    }

    static std::string parse_address(const c74::min::atom& address) {
        if (address.type() == c74::min::message_type::symbol_argument &&
            static_cast<std::string>(address).find('/') == 0) {

            return static_cast<std::string>(address);
        }

        throw std::runtime_error("the first argument must be a valid OSC address");
    }


private:
    AtomOscParser() = default;
};

#endif //PYOSC_ATOM_OSC_PARSER_H
