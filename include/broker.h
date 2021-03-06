/*
 ____            _               _     
| __ ) _ __ ___ | | _____ _ __  | |__  
|  _ \| '__/ _ \| |/ / _ \ '__| | '_ \ 
| |_) | | | (_) |   <  __/ | _  | | | |
|____/|_|  \___/|_|\_\___|_|(_) |_| |_|

Author: Killer Lorenz
Class : 5BHIF
Date  : 07-03-2019

*/

#pragma once

// +--------------------------+
// | Includes                 |
// +--------------------------+

#include <iostream>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <regex>

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
#include "spdlog/sinks/basic_file_sink.h"
#pragma GCC diagnostic pop

// +--------------------------+
// | Type Definitions         |
// +--------------------------+

typedef std::shared_ptr<asio::ip::tcp::socket>             shared_socket;
typedef std::map<std::string, std::vector<shared_socket>>  shared_socket_map;

// +--------------------------+
// | Class Declaration        |
// +--------------------------+
class Broker{
public:
    Broker(short unsigned int, std::string, std::string, std::string);
    void start();
    
private:
    short unsigned int       _port;
    shared_socket_map        _topics; 
    std::mutex               _topics_locker;
    std::string              _name; 
    std::string              _config;
    std::vector<std::string> _topics_s;
    std::string              _save;

    bool isValid(protobuf::Request&, shared_socket&);
    bool topicAllowed(std::string);
    std::vector<std::string> resolveWildcards(std::string);
    void removeFromAllTopics(shared_socket);

    protobuf::Request receiveRequest(shared_socket);
    void sendResponse(shared_socket, std::string, protobuf::Response_ResponseType, std::string);

    void serveClient(shared_socket);
};