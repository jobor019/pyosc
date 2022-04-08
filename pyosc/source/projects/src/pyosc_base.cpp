#include "pyosc_base.h"


BaseOscObject::BaseOscObject(const std::string& name
                             , const std::string& address
                             , const std::string& status_address)
        : name(name)
          , address(address)
          , status_address(format_address(status_address, address))
          , status(Status::offline) {
}


Status BaseOscObject::parse_status(c74::min::atoms& msg) {
    if (msg.size() != 1) {
        return Status::invalid_status;
    }

    int status_code;

    if (msg[0].type() == c74::min::message_type::int_argument) {
        status_code = static_cast<int>(msg[0]);
    } else {
        return Status::invalid_status;
    }

    if (status_code >= static_cast<int>(MIN_STATUS) && status_code <= static_cast<int>(MAX_STATUS)) {
        return static_cast<Status>(status_code);
    }

    return Status::invalid_status;
}

Status BaseOscObject::read_server_status() {
    if (!initialized) {
        status = Status::uninitialized;
        return status;
    } else {
        // initialize has been called but has not yet received any response from the server
        if (!last_response) {
            status = Status::initializing;
            last_response = std::chrono::system_clock::now();
            return status;
        }

        // Read status messages from receiver
        auto new_status_values = receive(status_address);

        // If status message received: set status to received message and update time
        if (!new_status_values.empty()) {
            auto last_status_candidate = new_status_values.back();
            status = parse_status(last_status_candidate);
            last_response = std::chrono::system_clock::now();

        } else if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() -
                                                                    *last_response).count() > TIMEOUT_SECONDS) {
            status = Status::no_response;
        }
    }
    return status;
}


const std::string& BaseOscObject::get_address() const {
    return address;
}

const std::string& BaseOscObject::get_status_address() const {
    return status_address;
}

std::string BaseOscObject::format_full_name(const std::string& name, const std::string& parent_name) {
    return parent_name + "::" + name;
}

std::string BaseOscObject::format_address(const std::string& base_address, const std::string& parent) {
    std::stringstream ss;
    if (parent.find('/') != 0) {
        ss << "/";
    }
    ss << parent;

    if (base_address.find('/') != 0) {
        ss << "/";
    }
    ss << base_address;

    return ss.str();
}

std::string BaseOscObject::address_from_full_name(const std::string& name) {
    std::stringstream ss;
    ss << "/";
    ss << std::regex_replace(name, std::regex("::"), "/");
    return ss.str();
}

bool BaseOscObject::is_initialized() const {
    return initialized;
}




