#include "broker.h"

using namespace std;
using namespace asio::ip;

Broker::Broker(short unsigned int port, string name) : 
    _port{port}, 
    _topics{make_shared<socket_map>()},
    _name{name} 
{}

void Broker::start(){

    asio::io_context ctx;                                               // IO Context
    tcp::endpoint    ep{tcp::v4(), _port};                              // Endpoint
    tcp::acceptor    acceptor{ctx, ep};                                 // Acceptor     

    acceptor.listen();  

    spdlog::get(_name)->info("Listening on port " + to_string(_port));
    
    while (true){

        shared_socket sock{make_shared<tcp::socket>(ctx)};    // Socket

        acceptor.accept(*sock);  // Blocking !!!

        spdlog::get(_name)->info("A new Client connected");

        thread t{&Broker::serveClient, this, move(sock)};
        t.detach();    
    }

    acceptor.close();
}

bool Broker::isValid(protobuf::Request& request){
    return !request.topic().empty();
}

protobuf::Request Broker::receiveRequest(shared_socket socket){
    // Read Data from Socket & write it into String 
    asio::streambuf b;
    asio::read_until(*socket, b, "ENDOFMESSAGE");     // Blocking !!!
    asio::streambuf::const_buffers_type bufs = b.data();
    string s{asio::buffers_begin(bufs),
             asio::buffers_begin(bufs) + b.size()}; 
    s.erase(s.size() - 12);
    
    // Parse Request from String
    protobuf::Request request;
    request.ParseFromString(s);

    return request;
}

void Broker::sendResponse(
    shared_socket socket, 
    string topic, 
    protobuf::Response_ResponseType type,
    string content){

    protobuf::Response response;

    response.set_topic(topic);
    response.set_type(type); 
    response.set_body(content);    

    string s;
    response.SerializeToString(&s);

    asio::write(*socket, asio::buffer(s + "ENDOFMESSAGE"));
}

void Broker::serveClient(shared_socket socket){
    while (true){

        //this_thread::sleep_for(chrono::seconds(10));

        protobuf::Request request{ 
            receiveRequest(socket)
        };

        if (isValid(ref(request))){

            int    type  = request.type();
            string topic = request.topic();

            // SUBSCRIBE
            if (type == protobuf::Request::SUBSCRIBE) {

                spdlog::get(_name)->info("Received Request : SUBSCRIBE " + topic);

                unique_lock lck{_topics_locker};

                if (_topics->count(topic) == 0){
                    _topics->emplace(
                        make_pair(topic, vector<shared_socket>())
                    );
                }

                _topics->at(topic).push_back(socket);

                lck.unlock();

                spdlog::get(_name)->info("Sending  Response: OK (SUBSCRIBE " + topic + ")");

                sendResponse(
                    socket, 
                    topic, 
                    protobuf::Response::OK,
                    "SUBSCRIBE " + topic
                );
            
            // UNSUBSCRIBE
            } else if (type == protobuf::Request::UNSUBSCRIBE) {
                
                spdlog::get(_name)->info("Received Request : UNSUBSCRIBE " + topic);

                unique_lock lck{_topics_locker};

                if (_topics->count(topic) != 0){

                    auto it = _topics->at(topic).begin();

                    while (it != _topics->at(topic).end()){

                        if (it->get() == socket.get()){
                            it = _topics->at(topic).erase(it);
                        }
                    }
                }

                lck.unlock();

                spdlog::get(_name)->info("Sending  Response: OK (UNSUBSCRIBE " + topic + ")");

                sendResponse(
                    socket, 
                    topic, 
                    protobuf::Response::OK,
                    "UNSUBSCRIBE " + topic
                );
            
            // PUBLISH
            } else if (type == protobuf::Request::PUBLISH) {

                spdlog::get(_name)->info("Received Request : PUBLISH " + topic + ": " + request.body());

                unique_lock lck{_topics_locker};

                if (_topics->count(topic) != 0){

                    for (shared_socket& sock : _topics->at(topic)){
                        sendResponse(
                            sock, 
                            topic, 
                            protobuf::Response::UPDATE,
                            request.body()
                        );
                    }
                }

                lck.unlock();

                spdlog::get(_name)->info("Sending  Response: OK (PUBLISH " + topic + ")");

                sendResponse(
                    socket, 
                    topic, 
                    protobuf::Response::OK,
                    "PUBLISH" + topic
                );

            } else {

                spdlog::get(_name)->error("Invalid Request Type!");

                sendResponse(
                    socket, 
                    "", 
                    protobuf::Response::ERROR,
                    "Invalid Request Type!"
                );
            }

        } else {
            spdlog::get(_name)->error("Invalid Request!");

            sendResponse(
                socket, 
                "", 
                protobuf::Response::ERROR,
                "Invalid Request!"
            );
        }    
    }
}

int main() {

    auto logger = spdlog::stdout_color_mt("Broker");

    Broker broker{6666, "Broker"};
    broker.start();
}