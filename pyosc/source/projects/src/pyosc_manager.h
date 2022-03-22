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

private:
    PyOscManager() {} // Private constructor

};