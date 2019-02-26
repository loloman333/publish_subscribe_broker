#pragma once

// --------------------------- Includes --------------------------------------------------------

#include <iostream>
#include <map>
#include <vector>
#include <thread>

// Protobuf
#include "messages.pb.h"

// Asio
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include "asio.hpp"
#pragma GCC diagnostic pop

// ------------------------------------------ Type Definitions ---------------------------------

typedef std::shared_ptr<asio::ip::tcp::socket>                             shared_socket;
typedef std::map<std::string, std::vector<shared_socket>>                  socket_map;
typedef std::shared_ptr<std::map<std::string, std::vector<shared_socket>>> shared_socket_map;

// -------------------------- Class -----------------------------------------------------------
class Broker{
public:
    Broker(short unsigned int);
    void start();
    
private:
    short unsigned int _port;
    shared_socket_map  _topics; 

    bool isValid(protobuf::Request&);

    protobuf::Request receiveRequest(shared_socket);
    void sendResponse(shared_socket, protobuf::Response&);

    void serveClient(shared_socket);
};