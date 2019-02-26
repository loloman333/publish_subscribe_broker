#include "client.h"

using namespace std;
using namespace asio::ip;
using json = nlohmann::json;

Client::Client(short unsigned int port) : _port{port}, _socket{_ctx}{}

void Client::sendRequest(protobuf::Request& request){
    string s;
    request.SerializeToString(&s);

    asio::write(_socket, asio::buffer(s + "ENDOFMESSAGE"));
}

protobuf::Response Client::receiveResponse(){
    // Read Data from Socket & write it into String 
    asio::streambuf b;
    asio::read_until(_socket, b, "ENDOFMESSAGE");     // Blocking !!!
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
        protobuf::Response response{
            receiveResponse()
        };

        cout << "Client: Received a Respond!" << endl;

        if (response.type() == protobuf::Response::OK){
            cout << "Client: Broker responded with OK!" << endl;
            cout << "Client: " + response.body() << endl;

        } else if (response.type() == protobuf::Response::ERROR){
            cout << "Client: Broker responded with ERROR!" << endl;
            cout << "Client: " + response.body() << endl;

        } else if (response.type() == protobuf::Response::UPDATE){

            cout << "Client: Received following Update to Topic " + response.topic() + ":" << endl;
            cout << response.body() << endl;
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
                cout << "Client: Sending Request to subsribe to Topic " + topic << endl;
            } else if (type_s == "UNSUBSCRIBE"){
                type = protobuf::Request::UNSUBSCRIBE;
                cout << "Client: Sending Request to unsubsribe from Topic " + topic << endl;
            } else if (type_s == "PUBLISH"){
                type = protobuf::Request::PUBLISH;
                cout << "Client: Sending Request to publish the following content to Topic " + topic + ":" << endl;
                cout << content << endl;
            } else {
                cout << "Client: Invalid 'type' in JSON Configuration File. Command will be ignored!" << endl;
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

    auto results = resolver.resolve("localhost", to_string(_port));  
    asio::connect(_socket, results);  

    cout << "Client: Connected to Server" << endl;

    thread t{&Client::handleResponses, this};

    ifstream ifs("../src/Client/configs/doorwatcher.json", ifstream::in);

    bool keepAlive;

    try {
        json json_file = json::parse(ifs);

        keepAlive = json_file.value("keep_alive", true);

        for (json& action : json_file.at("actions")){
            executeJSON(action);
        }
            
    } catch (json::parse_error& e){
        cout << "Client: Could not find or parse the JSON Configuration File!" << endl;
        return;
    } catch (json::exception& e){
        cout << "Client: Something with JSON went wrong!" << endl;
        cout << "Client: See 'description.txt' in the configs Folder to see the correct structure." << endl;
        cout << "Client: " << e.id << " | " << e.what() << endl;
        return;
    }

    if (keepAlive){
        t.join();
    } else {
        t.detach();
    }
}

int main(){
    Client client{6666};
    client.start();
}