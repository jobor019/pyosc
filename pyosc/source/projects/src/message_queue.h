//
// Created by Joakim Borg on 31/03/2022.
//

#ifndef PYOSC_MESSAGE_QUEUE_H
#define PYOSC_MESSAGE_QUEUE_H

#include <deque>
#include "osc_send_message.h"

class MessageQueue {
public:
    MessageQueue() = default;

    void add(std::unique_ptr<OscSendMessage> msg) {
        auto lock = std::lock_guard(mutex);
        messages.emplace_back(std::move(msg));
    }

    std::unique_ptr<OscSendMessage> pop() {
        auto lock = std::lock_guard(mutex);
        if (!messages.empty()) {
            auto front = std::move(messages.front());
            messages.pop_front();
            return std::move(front);
        }

        return nullptr;
    }

    void clear() noexcept {
        auto lock = std::lock_guard(mutex);
        messages.clear();
    }

    unsigned long size() noexcept {
        return messages.size();
    }

    bool empty() noexcept {
        return messages.empty();
    }

private:
    std::mutex mutex;

    std::deque<std::unique_ptr<OscSendMessage> > messages;
};

#endif //PYOSC_MESSAGE_QUEUE_H
