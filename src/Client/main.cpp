#include <iostream>
#include <vector>

// Protobuf
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

    this_thread::sleep_for(chrono::seconds(10));

    protobuf::Request request2; 

    request2.set_type(protobuf::Request::PUBLISH);
    request2.set_topic("Hello World!");
    request2.set_body("This is a test message, have fun with it!");

    string s2;
    request2.SerializeToString(&s2);

    asio::write(sock, asio::buffer(s2 + "ENDOFMESSAGE"));

    this_thread::sleep_for(chrono::seconds(10));

    // Read Data from Socket & write it into String 
    asio::streambuf b;
    asio::read_until(sock, b, "ENDOFMESSAGE");     // Blocking !!!
    asio::streambuf::const_buffers_type bufs = b.data();
    string s3{asio::buffers_begin(bufs),
             asio::buffers_begin(bufs) + b.size()}; 
    s.erase(s.size() - 12);
    
    // Parse Request from String
    protobuf::Response response;
    response.ParseFromString(s3);

    cout << "Client: Got a Response" << endl;
    cout << "Topic: " << response.topic() << endl;
    cout << "Content: " << response.body() << endl;
}