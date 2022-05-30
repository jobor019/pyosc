/// @file
///	@ingroup 	minexamples
///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#include "c74_min_unittest.h"     // required unit test header
#include "pyosc.argparse.cpp"    // need the source of our object so that we can access it

// Unit tests are written using the Catch framework as described at
// https://github.com/philsquared/Catch/blob/master/docs/tutorial.md

SCENARIO("object produces correct output") {
    ext_main(nullptr);    // every unit test must call ext_main() once to configure the class

//    GIVEN("An instance of our object") {
//
//        test_wrapper<server> an_instance;
//        server&              my_object = an_instance;
//
//        // check that default attr values are correct
//        REQUIRE((my_object.greeting == symbol("hello world")));
//
//        // now proceed to testing various sequences of events
//        WHEN("a 'bang' is received") {
//            my_object.bang();
//            THEN("our greeting is produced at the outlet") {
//                auto& output = *c74::max::object_getoutput(my_object, 0);
//                REQUIRE((output.size() == 1));
//                REQUIRE((output[0].size() == 1));
//                REQUIRE((output[0][0] == symbol("hello world")));
//            }
//        }
//    }
}


TEST_CASE("Creating Server") {
    std::shared_ptr<Server> server1;
    std::shared_ptr<Server> server2;
    std::shared_ptr<Server> server3;
    std::shared_ptr<Server> server4;
    std::shared_ptr<Server> server5;
    std::shared_ptr<Server> server6;
    std::shared_ptr<Server> server7;


    // Creating unique server with auto port
    std::string name = "server1";
    server1 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1", std::nullopt);
    REQUIRE(server1->get_send_port() == 7000);
    REQUIRE(server1->get_recv_port() == 7001);


    // Creating next unique server with auto port
    name = "server2";
    server2 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1", std::nullopt);
    REQUIRE(server2->get_send_port() == 7002);
    REQUIRE(server2->get_recv_port() == 7003);


    // Creating server with dup name:
    bool throws = false;
    try {
        name = "server1";
        server3 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1", std::nullopt);
    } catch (std::runtime_error& e) {
        throws = true;
    }
    REQUIRE(throws);


    // Creating server with taken send port (should work)
    name = "server4";
    server4 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1"
                                                      , std::make_optional(PortSpec{7000, 8000}));
    REQUIRE(server4->get_send_port() == 7000);
    REQUIRE(server4->get_recv_port() == 8000);


    // Creating server with taken recv port (socket blocked)
    throws = false;
    try {
        name = "server5";
        server5 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1"
                                                          , std::make_optional(PortSpec{8000, 7001}));
    } catch (std::runtime_error& e) {
        throws = true;
    }
    REQUIRE(throws);


    // Creating server with previously taken name and ports after deallocating
    PyOscManager::get_instance().remove_object(server1->get_name());
    server1.reset();
    name = "server1";
    server6 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1", std::make_optional(
            PortSpec{7000, 7001}));
    REQUIRE(server6->get_send_port() == 7000);
    REQUIRE(server6->get_recv_port() == 7001);
}


TEST_CASE("Sending messages from Server") {

    std::string address = "/server1";
    std::string name = "server1";

    std::shared_ptr<Server> server1 = nullptr;
    PyOscManager::get_instance().remove_object(name);

    // Creating unique server with auto port

    server1 = PyOscManager::get_instance().new_server(name, "/status", "terminate", "127.0.0.1", std::make_optional(
            PortSpec{7000, 7001}));
    REQUIRE(server1->get_send_port() == 7000);
    REQUIRE(server1->get_recv_port() == 7001);
    server1->update_status();
    REQUIRE(server1->get_status() == Status::uninitialized);

    std::string init_msg;
    server1->initialize(init_msg);
    server1->update_status();


    while (server1->get_status() != Status::ready) {
        REQUIRE(server1->get_status() == Status::initializing);  // Fail if timeout
        server1->update_status();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }


    SECTION("Single Roundtrip") {
        OscSendMessage msg{address};
        int data = 123;
        msg.add_string("round_trip");
        msg.add_int(data);

        auto t1 = std::chrono::high_resolution_clock::now();

        server1->send(msg);

        auto ret_msg = server1->receive(address);
        while (ret_msg.empty()) {
            ret_msg = server1->receive(address);
        }

        auto t2 = std::chrono::high_resolution_clock::now();

        std::cout << "Single Roundtrip: "
                  << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()
                  << " microseconds\n";

        REQUIRE((ret_msg.size() == 1
                 && ret_msg[0].size() == 1
                 && static_cast<int>(ret_msg[0][0]) == data));

        REQUIRE(std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() < 1000);


    } SECTION("Multi Roundtrip 50k") {
        // TODO: This only tests behaviour when waiting for each response. Don't forget to test it asynchronously
        int num_requested = 50000;
        int num_received = 0;
        long total_duration_us = 0.0;

        for (int i = 0; i < num_requested; ++i) {

            OscSendMessage msg{address};
            msg.add_string("round_trip");
            msg.add_int(i);

            auto t1 = std::chrono::high_resolution_clock::now();

            server1->send(msg);

            auto ret_msgs = server1->receive(address);
            while (ret_msgs.empty()) {
                ret_msgs = server1->receive(address);
            }

            auto t2 = std::chrono::high_resolution_clock::now();

            total_duration_us += std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

            for (auto& ret_msg: ret_msgs) {
                num_received += 1;
                REQUIRE(static_cast<int>(ret_msg[0]) == i);

            }
        }

        std::cout << "Roundtrip (" << num_requested << "): " << (total_duration_us / 1e6)
                  << " seconds (per message: " << (total_duration_us / (double) num_requested) << "us)\n";

        REQUIRE(num_received == num_requested);

    }
}

