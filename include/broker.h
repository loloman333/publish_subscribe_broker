#pragma once

// --------------------------- Includes --------------------------------------------------------

#include <iostream>
#include <map>
#include <vector>
#include <thread>
#include <mutex>

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

// ------------------------------------------ Type Definitions ---------------------------------

typedef std::shared_ptr<asio::ip::tcp::socket>                             shared_socket;
typedef std::map<std::string, std::vector<shared_socket>>                  socket_map;
typedef std::shared_ptr<std::map<std::string, std::vector<shared_socket>>> shared_socket_map;

// -------------------------- Class -----------------------------------------------------------
class Broker{
public:
    Broker(short unsigned int, std::string);
    void start();
    
private:
    short unsigned int _port;
    shared_socket_map  _topics; 
    std::mutex         _topics_locker;
    std::string        _name;     

    bool isValid(protobuf::Request&);

    protobuf::Request receiveRequest(shared_socket);
    void sendResponse(shared_socket, std::string, protobuf::Response_ResponseType, std::string);

    void serveClient(shared_socket);
};