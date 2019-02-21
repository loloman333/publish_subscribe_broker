#include <iostream>
#include <vector>

#include "messages.pb.h"

// Asio
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include "asio.hpp"
#pragma GCC diagnostic pop

using namespace std;
using namespace asio::ip;

int main() {
    
    asio::io_context ctx;                           // IO Context
    tcp::resolver resolver{ctx};                    // Resolver

    auto results = resolver.resolve("localhost", "6666");    

    tcp::socket sock{ctx};                          // Socket

    asio::connect(sock, results);
    
    protobuf::Request request;                      // Protobuf Request Object

    request.set_type(protobuf::Request::SUBSCRIBE);
    request.set_topic("Hello World!");

    string s;
    request.SerializeToString(&s);

    asio::write(sock, asio::buffer(s + "ENDOFMESSAGE"));

    this_thread::sleep_for(chrono::seconds(2));

    protobuf::Request request2; 

    request2.set_type(protobuf::Request::UNSUBSCRIBE);
    request2.set_topic("Hello World!");

    string s2;
    request2.SerializeToString(&s2);

    asio::write(sock, asio::buffer(s2 + "ENDOFMESSAGE"));

    this_thread::sleep_for(chrono::seconds(10));
}