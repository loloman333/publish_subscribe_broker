#include <iostream>

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

    asio::io_context ctx;                       // IO Context
    tcp::endpoint ep{tcp::v4(), 6666};          // Endpoint
    tcp::acceptor acceptor{ctx, ep};            // Acceptor

    acceptor.listen();  
    
    while (true){

        tcp::socket sock{ctx};                  // Socket

        acceptor.accept(sock);

        protobuf::Request req;                  // Protobuf Request Object
        
        // Read Data from Socket & write into String 
        asio::streambuf b;
        asio::read_until(sock, b, "ENDOFMESSAGE");
        asio::streambuf::const_buffers_type bufs = b.data();
        string s{asio::buffers_begin(bufs),
                 asio::buffers_begin(bufs) + b.size()}; 
        s.erase(s.size()-12);

        req.ParseFromString(s);

        cout << "Got Request" << endl;
        cout << "Type: "      << req.type() << endl;
        cout << "Topic: "     << req.topic() << endl;

        sock.close();
    }
    
    acceptor.close();
}
