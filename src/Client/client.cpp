#include "client.h"

using namespace std;
using namespace asio::ip;

Client::Client(short unsigned int port) : _port{port} {}

void Client::sendRequest(tcp::socket& socket, protobuf::Request& request){
    string s;
    request.SerializeToString(&s);

    asio::write(socket, asio::buffer(s + "ENDOFMESSAGE"));
}

protobuf::Response Client::receiveResponse(tcp::socket& socket){
    // Read Data from Socket & write it into String 
    asio::streambuf b;
    asio::read_until(socket, b, "ENDOFMESSAGE");     // Blocking !!!
    asio::streambuf::const_buffers_type bufs = b.data();
    string s{asio::buffers_begin(bufs),
             asio::buffers_begin(bufs) + b.size()}; 
    s.erase(s.size() - 12);
    
    // Parse Request from String
    protobuf::Response response;
    response.ParseFromString(s);

    return response;
}

void Client::start(){
        
    asio::io_context ctx;                           // IO Context
    tcp::resolver resolver{ctx};                    // Resolver

    auto results = resolver.resolve("localhost", "6666");    

    tcp::socket socket{ctx};                        // Socket

    asio::connect(socket, results);
    
    protobuf::Request request;                      // Protobuf Request Object
    

    request.set_type(protobuf::Request::SUBSCRIBE);
    request.set_topic("Hello World!");

    sendRequest(ref(socket), request);

    this_thread::sleep_for(chrono::seconds(10));

    request.set_type(protobuf::Request::PUBLISH);
    request.set_topic("Hello World!");
    request.set_body("This is a test message, have fun with it!");

    sendRequest(ref(socket), request);

    this_thread::sleep_for(chrono::seconds(10));

    protobuf::Response response{
        receiveResponse(ref(socket))
    };

    cout << "Client: Got a Response" << endl;
    cout << "Topic: " << response.topic() << endl;
    cout << "Content: " << response.body() << endl;
}

int main(){
    Client client{6666};
    client.start();
}