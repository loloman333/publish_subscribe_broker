#include <iostream>
#include <vector>
#include <map>
#include <thread>

// CLI11
// #include "CLI11.hpp"

// Asio
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include "asio.hpp"
#pragma GCC diagnostic pop

using namespace std;
using namespace asio::ip;

int main() {

    // Network Stuff

    asio::io_context ctx;                       // IO Context
    tcp::endpoint ep{tcp::v4(), 6666};          // Endpoint
    tcp::acceptor acceptor{ctx, ep};            // Acceptor

    acceptor.listen();  
    
    while (true){

        tcp::socket sock{ctx};                  // Socket

        acceptor.accept(sock);

        asio::streambuf buf;                    
        asio::read_until(sock, buf, '\n');

        string message;
        istream is{&buf};
        getline(is, message);

        cout << "Got Message: " << message << endl;

        sock.close();
    }
    
    acceptor.close();
}
