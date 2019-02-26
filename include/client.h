#pragma once

// --------------------------- Includes --------------------------------------------------------

#include <iostream>

// Protobuf
#include "messages.pb.h"

// Asio
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include "asio.hpp"
#pragma GCC diagnostic pop

// -------------------------- Class -----------------------------------------------------------
class Client{
public:
    Client(short unsigned int);
    void start();
    
private:
    short unsigned int _port;

    void sendRequest(asio::ip::tcp::socket&, protobuf::Request&);
    protobuf::Response receiveResponse(asio::ip::tcp::socket&);
};