#pragma once

// --------------------------- Includes --------------------------------------------------------

#include <iostream>
#include <fstream>

// JSON
#include "json.hpp"

// Protobuf
#include "messages.pb.h"

// Asio
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include "asio.hpp"
#pragma GCC diagnostic pop

// Spdlog
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#pragma GCC diagnostic pop

// -------------------------- Class -----------------------------------------------------------
class Client{
public:
    Client(short unsigned int, std::string);
    void start();
    
private:
    short unsigned int    _port;
    asio::io_context      _ctx;
    asio::ip::tcp::socket _socket;
    std::string           _name;

    void handleResponses();

    void sendRequest(protobuf::Request&);
    protobuf::Response receiveResponse();

    void executeJSON(nlohmann::json&);

};