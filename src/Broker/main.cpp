#include <iostream>
#include <map>

#include "messages.pb.h"

// Asio
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include "asio.hpp"
#pragma GCC diagnostic pop

using namespace std;
using namespace asio::ip;


bool isValid(protobuf::Request& request){
    return !request.topic().empty();
}

void serveClient(
        shared_ptr<tcp::socket> sock, 
        shared_ptr<map<string, vector<shared_ptr<tcp::socket>>>> topics) 
    {

    while (true){
        // Read Data from Socket & write it into String 
        asio::streambuf b;
        asio::read_until(*sock, b, "ENDOFMESSAGE");     // Blocking !!!
        asio::streambuf::const_buffers_type bufs = b.data();
        string s{asio::buffers_begin(bufs),
                 asio::buffers_begin(bufs) + b.size()}; 
        s.erase(s.size() - 12);

        // Parse Request from String
        protobuf::Request request;
        request.ParseFromString(s);

        cout << "Broker: Received a Request!" << endl;

        if (isValid(ref(request))){

            int    type  = request.type();
            string topic = request.topic();

            vector<shared_ptr<tcp::socket>> topicSocks;

            if (type == protobuf::Request::SUBSCRIBE) {

                if (topics->count(topic) == 0){
                    topics->emplace(
                        make_pair(topic, vector<shared_ptr<tcp::socket>>())
                    );
                }

                topics->at(topic).push_back(sock);

                cout << "Broker: Added Socket to Topic" << endl;
            

            } else if (type == protobuf::Request::UNSUBSCRIBE) {

                if (topics->count(topic) == 0){
                    break;
                }
                
                auto it = topics->at(topic).begin();

                while (it != topics->at(topic).end()){

                    if (it->get() == sock.get()){
                        it = topics->at(topic).erase(it);
                    }

                    cout << "Broker: Removed Socket from Topic" << endl;
                }
                
            } else {
                cout << "Broker: ERROR! Request didn't contain a valid Type!" << endl;
            }    
        } else {
            cout << "Broker: ERROR! Request didn't contain a valid Topic!" << endl;
            // TODO: Send back error message
            sock->close();
        }    
    }
}

int main() {

    asio::io_context ctx;                                               // IO Context
    tcp::endpoint    ep{tcp::v4(), 6666};                               // Endpoint
    tcp::acceptor    acceptor{ctx, ep};                                 // Acceptor

    shared_ptr<map<string, vector<shared_ptr<tcp::socket>>>> topics;    // Map with all Topics & Pointer to subscribed Sockets

    topics = make_shared<map<string, vector<shared_ptr<tcp::socket>>>>();

    acceptor.listen();  
    
    while (true){

        shared_ptr<tcp::socket> sock{make_shared<tcp::socket>(ctx)};    // Socket

        //cout << topics.use_count() << endl;

        acceptor.accept(*sock);    // Blocking !!!

        thread t{serveClient, move(sock), topics};
        t.detach();    
    }

    acceptor.close();
}