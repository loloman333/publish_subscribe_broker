#pragma once

// --------------------------- Includes --------------------------------------------------------

#include <iostream>
#include <fstream>

// JSON
#include "json.hpp"

// Protobuf
#include "messages.pb.h"

// CLI11
#include "CLI11.hpp"

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
    Client(std::string, short unsigned int, std::string, std::string);
    void start();
    
private:
    std::string           _hostname;
    short unsigned int    _port;
    std::string           _config;
    std::string           _name;

    asio::io_context      _ctx;
    asio::ip::tcp::socket _socket;

    void handleResponses();

    void sendRequest(protobuf::Request&);
    protobuf::Response receiveResponse();

    void executeJSON(nlohmann::json&);

};