#include "broker.h"

using namespace std;
using namespace asio::ip;

Broker::Broker(short unsigned int port) : _port{port}, _topics{make_shared<socket_map>()} {}

void Broker::start(){

    asio::io_context ctx;                                               // IO Context
    tcp::endpoint    ep{tcp::v4(), _port};                              // Endpoint
    tcp::acceptor    acceptor{ctx, ep};                                 // Acceptor     

    acceptor.listen();  
    
    while (true){

        shared_socket sock{make_shared<tcp::socket>(ctx)};    // Socket

        acceptor.accept(*sock);  // Blocking !!!

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

void Broker::sendResponse(shared_socket socket, protobuf::Response& response){
    string s;
    response.SerializeToString(&s);

    asio::write(*socket, asio::buffer(s + "ENDOFMESSAGE"));
}

void Broker::serveClient(shared_socket socket){
    while (true){

        protobuf::Request request{
            receiveRequest(socket)
        };
    
        cout << "Broker: Received a Request!" << endl;

        if (isValid(ref(request))){

            int    type  = request.type();
            string topic = request.topic();
            
            // SUBSCRIBE
            if (type == protobuf::Request::SUBSCRIBE) {

                if (_topics->count(topic) == 0){
                    _topics->emplace(
                        make_pair(topic, vector<shared_socket>())
                    );
                }

                _topics->at(topic).push_back(socket);

                cout << "Broker: Added Socket to Topic" << endl;
            
            // UNSUBSCRIBE
            } else if (type == protobuf::Request::UNSUBSCRIBE) {

                if (_topics->count(topic) == 0){
                    break;
                }
                
                auto it = _topics->at(topic).begin();

                while (it != _topics->at(topic).end()){

                    if (it->get() == socket.get()){
                        it = _topics->at(topic).erase(it);
                    }

                    cout << "Broker: Removed Socket from Topic" << endl;
                }
            
            // PUBLISH
            } else if (type == protobuf::Request::PUBLISH) {

                if (_topics->count(topic) == 0){
                    break;
                }

                protobuf::Response response;
                response.set_type(protobuf::Response::UPDATE);
                response.set_topic(topic);
                response.set_body(request.body());

                for (shared_socket& sock : _topics->at(topic)){
                    sendResponse(sock, response);
                }

                cout << "Broker: Published Content for Topic!" << endl;
            } else {
                cout << "Broker: ERROR! Request didn't contain a valid Type!" << endl;
            }

        } else {
            cout << "Broker: ERROR! Request didn't contain a valid Topic!" << endl;
            // TODO: Send back error message instead of closing it
        }    
    }
}

int main() {
    Broker broker{6666};
    broker.start();
}