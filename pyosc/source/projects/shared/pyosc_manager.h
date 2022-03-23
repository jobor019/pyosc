#ifndef PYOSC_PYOSC_MANAGER_H
#define PYOSC_PYOSC_MANAGER_H

#include <iostream>
#include <vector>
#include "connector.h"

// Singleton class for managing OSC ports and object names
class PyOscManager {
public:
    // Instantiation is thread-safe (c++11): see https://stackoverflow.com/a/335746
    static PyOscManager& get_instance() {
        // Guaranteed to be destroyed.
        static PyOscManager instance;
        // Instantiated on first use.
        return instance;
    }

    PyOscManager(PyOscManager const&) = delete;

    void operator=(PyOscManager const&) = delete;

    bool add_object(std::string& name) {
        return false;
    }

    std::shared_ptr<Connector> initialize_connector(std::string& object_name) {
        return nullptr;
    }

    std::shared_ptr<Connector> get_connector(std::string& owner) {
        return nullptr;
    }

    bool delete_object(std::string& name) {
        return false;
    }


private:
    PyOscManager() {} // Private constructor

    std::vector<int> free_ports;
    std::vector<std::string> object_names;
    std::map<std::string, std::shared_ptr<Connector>> connectors;


    std::mutex mutex;


};

#endif //PYOSC_PYOSC_MANAGER_H