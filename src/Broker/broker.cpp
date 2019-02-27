#include "broker.h"

using namespace std;
using namespace asio::ip;
using json = nlohmann::json;

Broker::Broker(short unsigned int port, string name, string config) : 
    _port{port}, 
    _name{name},
    _config{config} 
{}

void Broker::start(){
    
    // Read JSON Config File (if stated)
    if (_config != ""){

        ifstream ifs(_config, ifstream::in);

        try {
            json json_file = json::parse(ifs);

            for (json& topic : json_file){
                _topics_s.push_back(topic);
            }
                
        } catch (json::parse_error& e){
            spdlog::get(_name)->error("Could not find or parse the JSON Configuration File!");
            return;
        } catch (json::exception& e){
            spdlog::get(_name)->error("Something with JSON went wrong!");
            spdlog::get(_name)->error(to_string(e.id) + " | " + e.what());
            return;
        }
        spdlog::get(_name)->info("Configuration File successfully read. The following topics will be available:");
        for (string s : _topics_s){
            spdlog::get(_name)->info(s); 
        }
    } else {
        spdlog::get(_name)->info("No Configuration File was stated. Topics will be generated dynamically.");
    }

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

bool Broker::topicAllowed(string topic){
    if (_config == ""){
        return true;
    }
    
    for (string s : _topics_s){
        if (s == topic){
            return true;
        }
    }

    return false;
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

                if (_topics.count(topic) == 0){
                    if (topicAllowed(topic)){
                        _topics.emplace(
                            make_pair(topic, vector<shared_socket>())
                        );
                    } else {
                        spdlog::get(_name)->info("Sending  Response: ERROR (SUBSCRIBE " + topic + ")");

                        sendResponse(
                        socket, 
                        topic, 
                        protobuf::Response::ERROR,
                        "SUBSCRIBE " + topic + " This Topic is not allowed!"
                        );
                        continue;
                    }
                }

                _topics.at(topic).push_back(socket);

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

                if (_topics.count(topic) != 0){

                    auto it = _topics.at(topic).begin();

                    while (it != _topics.at(topic).end()){

                        if (it->get() == socket.get()){
                            it = _topics.at(topic).erase(it);
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

                if (_topics.count(topic) != 0){

                    for (shared_socket& sock : _topics.at(topic)){
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
                    "PUBLISH " + topic
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

int main(int argc, char* argv[]){

    // Command Line Interface
    CLI::App app{};

    unsigned short int port{6666};
    string             name{"Broker"};
    string             config{""};

    app.add_option("-p, --port",   port, "The Port the Broker will listen on (Default:  6666)");
    app.add_option("-n, --name",   name, "The Name of the Broker (Default: 'Broker')");
    app.add_option("-c, --config", config, "The Path to the JSON Config File (If this is not set topics will be created dynamically)");

    CLI11_PARSE(app, argc, argv);

    // Logger
    auto logger = spdlog::stdout_color_mt(name);

    // Start Broker
    Broker broker{6666, name, config};
    broker.start();
}