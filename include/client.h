#pragma once

// --------------------------- Includes --------------------------------------------------------

#include <iostream>
#include <fstream>
#include "json.hpp"

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
    short unsigned int    _port;
    asio::io_context      _ctx;
    asio::ip::tcp::socket _socket;

    void handleResponses();

    void sendRequest(protobuf::Request&);
    protobuf::Response receiveResponse();

    void executeJSON(nlohmann::json&);

};