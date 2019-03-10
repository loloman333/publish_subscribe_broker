/* 
 ____            _                                
| __ ) _ __ ___ | | _____ _ __    ___ _ __  _ __  
|  _ \| '__/ _ \| |/ / _ \ '__|  / __| '_ \| '_ \ 
| |_) | | | (_) |   <  __/ | _  | (__| |_) | |_) |
|____/|_|  \___/|_|\_\___|_|(_)  \___| .__/| .__/ 
                                     |_|   |_|    

Author: Killer Lorenz
Class : 5BHIF
Date  : 07-03-2019

*/

#include "broker.h"

using namespace std;
using namespace asio::ip;
using json = nlohmann::json;

// +--------------------------+
// | Helper Functions         |
// +--------------------------+

vector<string> splitString(string s, string delim){
    auto start = 0U;
    auto end = s.find(delim);

    vector<string> tokens;

    while (end != string::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + delim.length();
        end = s.find(delim, start);
        
    }
    tokens.push_back(s.substr(start, end));

    return tokens;
}

// +--------------------------+
// | Class Definition         |
// +--------------------------+

// Constructor
Broker::Broker(short unsigned int port, string name, string config, string save) : 
    _port{port}, 
    _name{name},
    _config{config},
    _save{save}
{}

// Reads the Config File and starts listening on the Port
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
            _topics.emplace(
                make_pair(s, vector<shared_socket>())
            );
        }

    } else {
        spdlog::get(_name)->warn("No Configuration File was stated. Topics will be generated dynamically.");
    }

    // Output File Logger Information
    if (_save == ""){
        spdlog::get(_name)->warn("No File for Logging was stated. Updates to Topics won't be saved to a File.");
    } else {
        spdlog::get(_name)->info("Updates for Topic will be saved to: " + _save);
    }

    // Asio Network Stuff
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

// Checks if a Protobuf Request is valid for a specific Client
bool Broker::isValid(protobuf::Request& request, shared_socket& socket){

    // Empty Topic ?
    if (request.topic().empty()){

        spdlog::get(_name)->info("Received Request with empty Topic!");
        spdlog::get(_name)->info("Sending  Response: ERROR (Topic can not be empty!)");
        sendResponse(
            socket, 
            "", 
            protobuf::Response::ERROR,
            "Topic can not be empty!"
        );
        return false;
    
    // Invalid Request Type ?
    } else if (request.type() != protobuf::Request::SUBSCRIBE &&
               request.type() != protobuf::Request::UNSUBSCRIBE &&
               request.type() != protobuf::Request::PUBLISH ){
        
        spdlog::get(_name)->info("Received Request with invalid Type!");
        spdlog::get(_name)->info("Sending  Response: ERROR (Request Type was invalid!)");
        sendResponse(
            socket, 
            "", 
            protobuf::Response::ERROR,
            "Request Type was invalid!"
        );
        return false;

    // Invalid Topic ?
    } else {

        // Cut leading "/"" if present
        char firstChar = request.topic().at(0);
        if (firstChar == '/'){
            request.set_topic(request.topic().substr(1));
        }
        bool valid = true;

        // Split string in tokens 
        vector<string> tokens = splitString(request.topic(), "/");        

        // Check if the tokens are valid 
        for (string& token : tokens){
            if (token == "") {
                valid = false;
                break;
            } else if (token.find('+') != string::npos || token.find('#') != string::npos) {
                if (token.size() != 1){
                    valid = false;
                } else if (request.type() == protobuf::Request::PUBLISH){
                    spdlog::get(_name)->info("Received Request: PUBLISH with wildcard in Topic!");
                    spdlog::get(_name)->info("Sending  Response: ERROR (Can not use wildcard in Topic with PUBLISH!)");
                    sendResponse(
                        socket, 
                        "", 
                        protobuf::Response::ERROR,
                        "Can not use wildcard in Topic with PUBLISH!"
                    );
                    return false;;
                }
            }  
        }

        // If something wasn't valid -> inform Client & return false 
        if (!valid){ 
            spdlog::get(_name)->info("Received Request with invalid Topic!");
            spdlog::get(_name)->info("Sending  Response: ERROR (Topic was invalid!)");
            sendResponse(
                socket, 
                "", 
                protobuf::Response::ERROR,
                "Topic was invalid: " + request.topic()
            );
            return valid;
        } 
    }
    return true;
}

// Checls if a Topic is allowed with the current config
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

// Receives a Protobuf Request from a specific Client
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

// Sends a Protobuf Response to a specific Client
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

    try{
        asio::write(*socket, asio::buffer(s + "ENDOFMESSAGE"));
    } catch (std::system_error& e) {
        if (e.code().value() != 32){
            spdlog::get(_name)->warn("An Error occured while reading from a Socket!");
            throw e;
        }
    }


    this_thread::sleep_for(chrono::milliseconds(1)); // ???
}

// Resolves a Topic string with Wildcards into a Vector of Topics
vector<string> Broker::resolveWildcards(string topicWildcard){
    vector<string> resolved;

    if (topicWildcard.find("+") != string::npos || topicWildcard.find("#") != string::npos){

        while (topicWildcard.find("+") != string::npos){
            topicWildcard.replace(
                topicWildcard.find("+"), 
                1, 
                "[^/]*"
            );
        }

        while (topicWildcard.find("#") != string::npos){
            topicWildcard.replace(
                topicWildcard.find("#"), 
                1, 
                "[^]*"
            );
        }

        auto rgx = regex(topicWildcard); 

        for (auto& pair : _topics){
            if (regex_match(pair.first, rgx)){
                resolved.push_back(pair.first);
            }
        }

        if (resolved.size() == 0){
            spdlog::get(_name)->info("Request Topic contains Wildcards but there are no matching Topics!");
        } else {
            spdlog::get(_name)->info("Request Topic contains Wildcards and includes the following Topics: ");
            for (string& s : resolved){
                spdlog::get(_name)->info(s);
            }
        }
    } else {
        resolved.push_back(topicWildcard);
    }

    return resolved;
}

// Removes a specific Client from all Topics
void Broker::removeFromAllTopics(shared_socket socket){
    unique_lock lck{_topics_locker};

    for (auto& topicPair : _topics){

        if (_topics.count(topicPair.first) != 0){

            auto it = _topics.at(topicPair.first).begin();

            while (it != _topics.at(topicPair.first).end()){

                if (it->get() == socket.get()){
                    it = _topics.at(topicPair.first).erase(it);
                } else {
                     it = it + 1;
                }
            }
        }
    }
}

// Constantly reads and executes Requests for a specific Client
void Broker::serveClient(shared_socket socket){

    while (true){

        protobuf::Request request;

        try {
            request = receiveRequest(socket);
        } catch (std::system_error& e) {
            if (e.code().value() == 2){
                spdlog::get(_name)->info("A Client disconnected");
                removeFromAllTopics(socket);
                break;
            } else {
                spdlog::get(_name)->error("An Error occured while reading from a Socket!");
                throw e;
            }
        }

        if (isValid(ref(request), ref(socket))){

            int    type  = request.type();
            string topic = request.topic();

            // SUBSCRIBE
            if (type == protobuf::Request::SUBSCRIBE) {

                spdlog::get(_name)->info("Received Request : SUBSCRIBE " + topic);

                unique_lock lck{_topics_locker};

                vector<string> resolvedTopics = resolveWildcards(topic);

                if (resolvedTopics.size() == 0){
                    spdlog::get(_name)->info("Sending  Response: ERROR (SUBSCRIBE " + topic + ": No matching Topics exist!)");
                    sendResponse(
                        socket, 
                        topic, 
                        protobuf::Response::ERROR,
                        "SUBSCRIBE " + topic + ": No matching Topics exist!"
                    );
                    continue;
                }

                for (string& curTopic : resolvedTopics){

                    if (_topics.count(curTopic) == 0){
                        if (topicAllowed(curTopic)){
                            _topics.emplace(
                                make_pair(curTopic, vector<shared_socket>())
                            );
                        } else {
                            spdlog::get(_name)->info("Sending  Response: ERROR (SUBSCRIBE " + curTopic + ": This Topic does not exist!)");

                            sendResponse(
                            socket, 
                            curTopic, 
                            protobuf::Response::ERROR,
                            "SUBSCRIBE " + curTopic + ": This Topic does not exist!"
                            );
                            continue;
                        }
                    }

                    _topics.at(curTopic).push_back(socket);

                    spdlog::get(_name)->info("Sending  Response: OK (SUBSCRIBE " + curTopic + ")");

                    sendResponse(
                        socket, 
                        topic, 
                        protobuf::Response::OK,
                        "SUBSCRIBE " + curTopic
                    );
                }

                lck.unlock();

            // UNSUBSCRIBE
            } else if (type == protobuf::Request::UNSUBSCRIBE) {
                
                spdlog::get(_name)->info("Received Request : UNSUBSCRIBE " + topic);

                unique_lock lck{_topics_locker};

                vector<string> resolvedTopics = resolveWildcards(topic);

                if (resolvedTopics.size() == 0){
                    spdlog::get(_name)->info("Sending  Response: ERROR (SUBSCRIBE " + topic + ": No matching Topics exist!)");
                    sendResponse(
                        socket, 
                        topic, 
                        protobuf::Response::ERROR,
                        "SUBSCRIBE " + topic + ": No matching Topics exist!"
                    );
                    continue;
                }

                for (string& curTopic: resolvedTopics){

                    if (_topics.count(curTopic) != 0){

                        auto it = _topics.at(curTopic).begin();

                        while (it != _topics.at(curTopic).end()){

                            if (it->get() == socket.get()){
                                it = _topics.at(curTopic).erase(it);
                            } else {
                                it = it + 1;
                            }
                        }
                    }

                    spdlog::get(_name)->info("Sending  Response: OK (UNSUBSCRIBE " + curTopic + ")");

                    sendResponse(
                        socket, 
                        curTopic, 
                        protobuf::Response::OK,
                        "UNSUBSCRIBE " + topic
                    );          
                }

                lck.unlock();
            
            // PUBLISH
            } else if (type == protobuf::Request::PUBLISH) {

                spdlog::get(_name)->info("Received Request : PUBLISH " + topic + ": " + request.body());

                unique_lock lck{_topics_locker};

                if (_topics.count(topic) != 0){

                    if (_save != ""){                        
                        spdlog::get(_name + " File Logger")->info("[" + topic + "] " + request.body());
                        spdlog::get(_name + " File Logger")->flush();
                    }
                    
                    for (shared_socket& sock : _topics.at(topic)){

                        if (sock.get() != socket.get()){
                            sendResponse(
                                sock, 
                                topic, 
                                protobuf::Response::UPDATE,
                                request.body()
                            );
                        }
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
            }
        }   
    }
}

// +--------------------------+
// | Main                     |
// +--------------------------+

int main(int argc, char* argv[]){

    // Command Line Interface
    CLI::App app{};

    unsigned short int port{6666};
    string             name{"Broker"};
    string             config{""};
    string             save{""};

    app.add_option("-p, --port",   port,   "The Port the Broker will listen on      (Default:  6666)");
    app.add_option("-n, --name",   name,   "The Name of the Broker                  (Default: 'Broker')");
    app.add_option("-c, --config", config, "The Path to the JSON Configuration File (Default: None)");
    app.add_option("-l, --log",    save,   "The Path to the Logfile                 (Default: None)");

    CLI11_PARSE(app, argc, argv);

    // Logger
    auto logger = spdlog::stdout_color_mt(name);

    if (save != ""){
        try {  
            auto file_logger = spdlog::basic_logger_mt(name + " File Logger", save);
        } catch (const spdlog::spdlog_ex &ex) {
            logger->error("Could not generate or find the Save File!");
            return 1;
        }
    }

    // Start Broker
    Broker broker{6666, name, config, save};
    broker.start();
}