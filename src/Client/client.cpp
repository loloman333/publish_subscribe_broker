#include "client.h"

using namespace std;
using namespace asio::ip;
using json = nlohmann::json;

Client::Client(string hostname, short unsigned int port, string config, string name) :
    _hostname{hostname},
    _port{port},
    _config{config},
    _name{name},
    _socket{_ctx}
{}

void Client::sendRequest(protobuf::Request& request){
    string s;
    request.SerializeToString(&s);

    asio::write(_socket, asio::buffer(s + "ENDOFMESSAGE"));    

    this_thread::sleep_for(chrono::milliseconds(1)); // ???
}

protobuf::Response Client::receiveResponse(){
    // Read Data from Socket & write it into String 
    asio::streambuf b;
    asio::read_until(_socket, b, "ENDOFMESSAGE");  // Blocking !!!
    asio::streambuf::const_buffers_type bufs = b.data();
    string s{asio::buffers_begin(bufs),
             asio::buffers_begin(bufs) + b.size()}; 
    s.erase(s.size() - 12);
    
    // Parse Request from String
    protobuf::Response response;
    response.ParseFromString(s);

    return response;
}

void Client::handleResponses(){
    while (true){

        protobuf::Response response;

        try {
            response = receiveResponse();
        } catch (std::system_error& e) {
            if (e.code().value() == 2){
                spdlog::get(_name)->error("The Connection to the Server was broken!");
                quick_exit(EXIT_SUCCESS);
            } else {
                spdlog::get(_name)->error("An Error occured while reading from a Socket!");
                throw e;
            }
        }

        if (response.type() == protobuf::Response::OK){
            spdlog::get(_name)->info("Received Response: OK (" + response.body() + ")");

        } else if (response.type() == protobuf::Response::ERROR){
            spdlog::get(_name)->warn("Received Response: ERROR (" + response.body() + ")");

        } else if (response.type() == protobuf::Response::UPDATE){
            spdlog::get(_name)->info("Received Update for " + response.topic() + ": " + response.body());
        }
    }
}

void Client::executeJSON(json& action){

    //Check > 0 (both)
    int repeat        = action.value("repeat", 1);          // What if string? What if float?
    int delay_after   = action.value("delay_after",  0);
    int delay_between = action.value("delay_between",  0);
   
    for (int i = 0; i < repeat; i++){

        for (json& command : action.value("commands", json::array())){

            string type_s  = command.at("type");
            string topic   = command.at("topic");
            string content = command.value("content", "");

            protobuf::Request::RequestType type;  

            if (type_s == "SUBSCRIBE"){
                type = protobuf::Request::SUBSCRIBE;
                spdlog::get(_name)->info("Sending  Request : SUBSCRIBE " + topic);
            } else if (type_s == "UNSUBSCRIBE"){
                type = protobuf::Request::UNSUBSCRIBE;
                spdlog::get(_name)->info("Sending  Request : UNSUBSCRIBE " + topic);
            } else if (type_s == "PUBLISH"){
                type = protobuf::Request::PUBLISH;
                spdlog::get(_name)->info("Sending  Request : PUBLISH " + topic + ": " + content);
            } else {
                spdlog::get(_name)->warn("Invalid Value for Field 'type' in JSON Configuration File. Command will be ignored!");
                continue;
            }
            
            protobuf::Request request;

            request.set_type(type);
            request.set_topic(topic);
            request.set_body(content);

            sendRequest(request);

            this_thread::sleep_for(chrono::milliseconds(delay_between));
        } 
        this_thread::sleep_for(chrono::milliseconds(delay_after));
    }
}

void Client::start(){

    tcp::resolver resolver{_ctx};                    // Resolver

    try {
        auto results = resolver.resolve(_hostname, to_string(_port));  
        asio::connect(_socket, results);  
    } catch (system_error& e){
        if (e.code().value() == 111){
            spdlog::get(_name)->error("Could not connect to the Server!");
            return;
        } else {
            spdlog::get(_name)->error("An Error occured while trying to connect to the Server!");
            throw e;
        }
    } 

    spdlog::get(_name)->info("Connected to Server at " + _hostname + ":" + to_string(_port));

    thread t{&Client::handleResponses, this};

    ifstream ifs(_config, ifstream::in);

    bool keepAlive;

    try {
        json json_file = json::parse(ifs);

        keepAlive = json_file.value("keep_alive", true);

        try{       
            for (json& action : json_file.at("actions")){
                executeJSON(action);
            }
        } catch (std::system_error& e) {
            if (e.code().value() == 32){
                spdlog::get(_name)->error("The Connection to the Server was broken!");
                quick_exit(EXIT_SUCCESS);
            } else {
                spdlog::get(_name)->error("An Error occured while reading from a Socket!");
                throw e;
            }
        }

            
    } catch (json::parse_error& e){
        spdlog::get(_name)->error("Could not find or parse the JSON Configuration File!");
        return;
    } catch (json::exception& e){
        spdlog::get(_name)->error("Something with JSON went wrong!");
        spdlog::get(_name)->error(to_string(e.id) + " | " + e.what());
        spdlog::get(_name)->info("See 'description.txt' in the configs Folder to see the correct JSON structure.");
        return;
    }

    if (keepAlive){
        t.join();
    } else {
        t.detach();
    }
}

int main(int argc, char* argv[]){

    // Command Line Interface
    CLI::App app{};

    unsigned short int port{6666};
    string             hostname{"localhost"};
    string             configFile;
    string             name{"Client"};

    app.add_option("-s, --server", hostname, "The Hostname the Client will connect to (Default:  'localhost')"); 
    app.add_option("-p, --port",   port, "The Port the Client will connect to (Default:  6666)"); 
    app.add_option("-c, --config", configFile, "The Path to the Config File")->required();
    app.add_option("-n, --name",   name, "The Name of the Client (Default: 'Client')");

    CLI11_PARSE(app, argc, argv);

    // Logger
    auto logger = spdlog::stdout_color_mt(name);

    // Start the Client
    Client client{hostname, port, configFile, name};
    client.start();
}