#include "pyosc_base.h"


BaseOscObject::BaseOscObject(const std::string& name
                             , const std::string& address
                             , const std::string& status_address)
        : name(name)
          , address(address)
          , status_address(status_address)
          , status(Status::offline) {
}


Status BaseOscObject::parse_status(osc::ReceivedMessage& msg) {
    if (msg.ArgumentCount() != 1) {
        return Status::invalid_status;
    }

    int status_code;
    auto arg = msg.ArgumentsBegin();

    if (arg->IsInt32()) {
        status_code = static_cast<int>(arg->AsInt32());
    } else if (arg->IsInt64()) {
        status_code = static_cast<int>(arg->AsInt64());
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


