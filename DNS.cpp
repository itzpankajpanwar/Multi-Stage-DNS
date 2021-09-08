#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h> 
#include <deque>
#include <cstring>
#define BUFFER_SIZE 1024
using namespace std;

int PORT;
vector<pair<string,string>> mapping;

int printError(string errorMessage){
    cout << errorMessage << endl;
    exit(-1);
}

void findDomainName(char* ip_address, char* domain_name){
    string ip_addr(ip_address);
    domain_name[0] = '3';
    domain_name[1] = '$';
    
    // searching for the entry in the database
    for(auto x:mapping){
        if(x.first == ip_addr){
            cout << "Found matching entry in the database: ";
            strcpy(domain_name+2, x.second.c_str());
            return;
        }
    }

    domain_name[0] = '4';
    string msg = "No entry found in the database!";
    strcpy(domain_name+2, msg.c_str());
}

void findIPAddress(char* domain_name, char* ip_address){
    string domain(domain_name);
    ip_address[0] = '3';
    ip_address[1] = '$';

    // searching for the entry in the database
    for(auto x:mapping){
        if(x.second == domain){
            cout << "Found matching entry in the database: ";
            strcpy(ip_address+2, x.first.c_str());
            return;
        }
    }

    ip_address[0] = '4';
    string msg = "No entry found in the database!";
    strcpy(ip_address+2, msg.c_str());
}

// reading IP address to domain name mapping from database
void readDatabase(){
    ifstream ifs("database.txt");
    while(!ifs.eof()){
        string ip_address, domain_name;
        ifs >> ip_address >> domain_name;
        mapping.push_back(make_pair(ip_address,domain_name));
    }
}

int main(int argc, char* argv[]){
    if(argc != 2) printError("Invalid number of arguments!");
    PORT = atoi(argv[1]);
    readDatabase();

    // creating new socket for communication
    cout << "Creating new socket..." << endl;
    int socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_server == -1) printError("Could not create socket!");
    else cout << "Socket created successfully." << endl;

    // configuring socket to reuse IP address and port number
    int opt = 1;
    if(setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) 
        printError("Error in configuring socket!");

    // configuring socket pair attributes
    struct sockaddr_in socket_address;
    int socket_address_len = sizeof(socket_address);
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons(PORT);

    // binding IP address and port number 
    cout << "Binding IP address and port number..." << endl;
    if(bind(socket_server, (struct sockaddr *)&socket_address,  sizeof(socket_address)) == -1) 
        printError("Error in binding IP address & port!");
    else cout << "Binding successful." << endl;
    
    // waiting for connection requests
    if(listen(socket_server, 3) == -1) printError("Listen error!");
    else cout << "Listening to port " << PORT << endl;
    cout << "-------------------------------------------\n" << endl;

    while(true){
        // establishing connection with the client
        int new_socket = accept(socket_server, (struct sockaddr *)&socket_address,  (socklen_t *)&socket_address_len);
        cout << "Connection request received" << endl;
        if(new_socket == -1) printError("Accept failure!");
        else cout << "Connection successful." << endl;

        // retrieving DNS query
        char buffer_recv[BUFFER_SIZE] = {0};
        read(new_socket, buffer_recv, BUFFER_SIZE);
        if(buffer_recv[0] == '9') break;

        // resolving DNS query
        char buffer_send[BUFFER_SIZE] = {0};
        cout << "Searching for the entry in the database..." << endl;
        if(buffer_recv[0] == '1') findIPAddress(buffer_recv+2, buffer_send);
        else findDomainName(buffer_recv+2, buffer_send);
        cout << buffer_send << endl;
        
        write(new_socket, buffer_send, BUFFER_SIZE);
        close(new_socket);
        cout << "-----------------------------------------------" << endl;
    }

    // closing connection
    cout << "Closing connection..." << endl;
    close(socket_server);
    cout << "Connection closed" <<  endl;
    return 0;
}