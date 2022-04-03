/// @file
///	@ingroup 	minexamples
///	@copyright	Copyright 2018 The Min-DevKit Authors. All rights reserved.
///	@license	Use of this source code is governed by the MIT License found in the License.md file.

#include "c74_min_unittest.h"     // required unit test header
#include "pyosc.server.cpp"    // need the source of our object so that we can access it
//#include "../src/connector.h"

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


SCENARIO("receiving multiple messages from server") {

    Connector c{"127.0.0.1", 7000, 7001};
//
//    std::stringstream ss;
//    ss << "Created connector (send_port=" << c.get_send_port() << ", recv_port=" <<c.get_recv_port() << ")\n";
////    INFO(ss.str());
//    std::cout << ss.str() << "\n";

//    c74::min::atoms atms;
//    atms.push_back({123});
//    std::cout << atms.at(0) << "\n";

//    while (true) {
//
//        auto msgs = c.receive("/s1/status");
//
//        for (auto& msg: msgs) {
////            std::cout << msg.AddressPattern() <<
//        }
//        std::this_thread::sleep_for(std::chrono::seconds(1));
//
//
//    }
}
