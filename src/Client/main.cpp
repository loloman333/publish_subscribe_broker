#include <iostream>

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
    tcp::resolver resolver{ctx};                // Resolver

    auto results = resolver.resolve("localhost", "6666");    

    tcp::socket sock{ctx};                      // Socket

    asio::connect(sock, results);

    asio::write(sock, asio::buffer("Hello World!\n"));
}