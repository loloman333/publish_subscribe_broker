#include "broker.h"

using namespace std;
using namespace asio::ip;

Broker::Broker(short unsigned int port) : _port{port}, _topics{make_shared<socket_map>()} {}

void Broker::start(){

    asio::io_context ctx;                                               // IO Context
    tcp::endpoint    ep{tcp::v4(), _port};                              // Endpoint
    tcp::acceptor    acceptor{ctx, ep};                                 // Acceptor     

    acceptor.listen();  

    cout << "Broker: Now listening on port " + to_string(_port) << endl;
    
    while (true){

        shared_socket sock{make_shared<tcp::socket>(ctx)};    // Socket

        acceptor.accept(*sock);  // Blocking !!!

        cout << "Broker: A new Client connected" << endl;

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

                cout << "Broker: Received Request to add Client to Topic " + topic << endl;

                unique_lock lck{_topics_locker};

                if (_topics->count(topic) == 0){
                    _topics->emplace(
                        make_pair(topic, vector<shared_socket>())
                    );
                }

                _topics->at(topic).push_back(socket);

                lck.unlock();

                cout << "Broker: Added Client to Topic " + topic << endl;

                sendResponse(
                    socket, 
                    topic, 
                    protobuf::Response::OK,
                    "Successfully subscribed to Topic " + topic
                );
            
            // UNSUBSCRIBE
            } else if (type == protobuf::Request::UNSUBSCRIBE) {

                cout << "Broker: Received Request to remove Client from Topic " + topic << endl;

                bool removed = false;

                unique_lock lck{_topics_locker};

                if (! _topics->count(topic) == 0){

                    auto it = _topics->at(topic).begin();

                    while (it != _topics->at(topic).end()){

                        if (it->get() == socket.get()){
                            it = _topics->at(topic).erase(it);
                            removed = true;
                        }
                    }
                }

                lck.unlock();

                if (removed){   
                    cout << "Broker: Removed Client from Topic " + topic << endl;

                    sendResponse(
                        socket, 
                        topic, 
                        protobuf::Response::OK,
                        "Successfully unsubscribed from Topic " + topic
                    );
                } else {
                    cout << "Broker: Client wasn't subscribed to Topic " + topic << endl;

                    sendResponse(
                        socket, 
                        topic, 
                        protobuf::Response::OK,
                        "No need to unsubscribe from Topic " + topic + ". You were not subscribed to it."
                    );
                }
            
            // PUBLISH
            } else if (type == protobuf::Request::PUBLISH) {

                cout << "Broker: Received Request to publish the following Content to Topic " + topic << endl;
                cout << request.body() << endl;

                unique_lock lck{_topics_locker};

                if (_topics->count(topic) == 0){
                    cout << "Broker: Nobody subscribed to Topic" + topic + ". Content will not be published." << endl;

                    sendResponse(
                        socket, 
                        topic, 
                        protobuf::Response::OK,
                        "Nobody was subscribed to Topic " + topic + ". Content was not published."
                    );
                    break;
                }

                for (shared_socket& sock : _topics->at(topic)){
                    sendResponse(
                        sock, 
                        topic, 
                        protobuf::Response::UPDATE,
                        request.body()
                    );
                }

                lck.unlock();

                cout << "Broker: Published the Content to Topic:" + topic << endl;
                sendResponse(
                    socket, 
                    topic, 
                    protobuf::Response::OK,
                    "Successfully published Content to Topic" + topic
                );

            } else {
                cout << "Broker: ERROR! Request didn't contain a valid Type!" << endl;

                sendResponse(
                    socket, 
                    "", 
                    protobuf::Response::ERROR,
                    "Request didn't contain a valid Type!"
                );
            }

        } else {
            cout << "Broker: ERROR! Request was not valid!" << endl;

            sendResponse(
                socket, 
                "", 
                protobuf::Response::ERROR,
                "Request was not valid!"
            );
        }    
    }
}

int main() {
    Broker broker{6666};
    broker.start();
}