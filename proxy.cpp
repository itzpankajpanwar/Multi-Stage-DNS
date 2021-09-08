#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h> 
#include <deque>
#include <cstring>
#define MAX_SIZE 3
#define BUFFER_SIZE 1024
using namespace std;

deque<pair<string,string>> cache;
int cache_size = 0;
int PROXY_PORT, DNS_PORT;

int printError(string errorMessage){
    cout << errorMessage << endl;
    exit(-1);
}

void updateCache(string ip_address, string domain_name){
    if(cache_size == MAX_SIZE){
        cache.pop_front();
        cache_size--;
    }
    cache.push_back(make_pair(ip_address, domain_name));
    cache_size++;
}

void connectDNSServer(char* buffer_request, char* buffer_reply, char* DNS_IP_address){
    // creating new socket for communication
    cout << "Creating new client socket..." << endl;
    int socket_client = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_client == -1) printError("Error creating client socket!");
    else cout << "Client socket created successfully." << endl;

    // configuring socket to reuse IP address and port number
    int opt = 1;
    if(setsockopt(socket_client, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) 
        printError("Error in configuring socket!");

    // configuring socket pair attributes
    struct sockaddr_in DNS_server_address;
    DNS_server_address.sin_family = AF_INET;
    // if(inet_pton(AF_INET, DNS_IP_address, &DNS_server_address) == -1) printError("Invalid IP address!");
    DNS_server_address.sin_addr.s_addr = inet_addr(DNS_IP_address);
    DNS_server_address.sin_port = htons(DNS_PORT);

    // sending connection request to DNS server
    cout << "Connecting to the DNS server..." <<  endl;
    int conn_status = connect(socket_client, (struct sockaddr *)&DNS_server_address, sizeof(DNS_server_address));
    if(conn_status == -1) printError("Error connecting to the DNS server!");
    else cout << "Connected to the DNS server." << endl;

    // sending DNS query to DNS server
    cout << "Sending request to DNS server..." << endl;
    write(socket_client, buffer_request, BUFFER_SIZE);
    read(socket_client, buffer_reply, BUFFER_SIZE);
    close(socket_client);
}

void findDomainName(char* ip_address, char* domain_name, char* DNS_IP_address){
    string ip_addr(ip_address+2);
    domain_name[0] = '3';
    domain_name[1] = '$';

    // searching for the entry in the cache
    for(auto mapping:cache){
        if(mapping.first == ip_addr){
            cout << "Found matching entry in the cache: ";
            strcpy(domain_name+2, mapping.second.c_str());
            return;
        }
    }

    // forwarding request to DNS server
    cout << "No entry found in the cache!" << endl;
    cout << "Forwarding request to DNS server..." << endl;
    connectDNSServer(ip_address, domain_name, DNS_IP_address);    
    cout << "Reply from DNS server: ";
    if(domain_name[0] == '3') updateCache(ip_addr, string(domain_name+2));
}

void findIPAddress(char* domain_name, char* ip_address, char* DNS_IP_address){
    string domain(domain_name+2);
    ip_address[0] = '3';
    ip_address[1] = '$';

    // searching for the entry in cache
    for(auto mapping:cache){
        if(mapping.second == domain){
            cout << "Found matching entry in the cache: ";
            strcpy(ip_address+2, mapping.first.c_str());
            return;
        }
    }

    // forwarding request to the DNS server
    cout << "No entry found in the cache!" << endl;
    cout << "Forwarding request to DNS server..." << endl;
    connectDNSServer(domain_name, ip_address, DNS_IP_address);
    cout << "Reply from DNS server: ";
    if(ip_address[0] == '3') updateCache(string(ip_address+2), domain);
}

int main(int argc, char* argv[]){
    if(argc != 4) printError("Invalid number of arguments!");
    PROXY_PORT = atoi(argv[1]);
    DNS_PORT = atoi(argv[3]);

    // creating new socket for communication
    cout << "Creating new socket..." << endl;
    int socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_server == -1) printError("Could not create socket!");
    else cout << "Socket created successfully." << endl;

    // configuring socket to IP address and port number
    int opt = 1;
    if(setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) 
        printError("Error in configuring socket!");

    // setting socket pair attributes
    struct sockaddr_in socket_address;
    int socket_address_len = sizeof(socket_address);
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons(PROXY_PORT);

    // binding IP address and port number
    cout << "Binding IP address and port number..." << endl;
    if(bind(socket_server, (struct sockaddr *)&socket_address,  sizeof(socket_address)) == -1) 
        printError("Error in binding IP address & port!");
    else cout << "Binding successful." << endl;
    
    // waiting for connection requests from client
    if(listen(socket_server, 3) == -1) printError("Listen error!");
    else cout << "Listening to the port " << PROXY_PORT << endl;

    // establishing connection with the client
    int new_socket = accept(socket_server, (struct sockaddr *)&socket_address, (socklen_t *)&socket_address_len);
    if(new_socket == -1) printError("Accept failure!");
    else cout << "Connection successful." << endl;
    cout << "------------------------------------------------------\n" << endl;

    while(true){
        // retrieving DNS query
        char buffer_recv[BUFFER_SIZE] = {0};
        char buffer_send[BUFFER_SIZE] = {0};
        read(new_socket, buffer_recv, BUFFER_SIZE);
        cout << "Request from client received" << endl;

        if(buffer_recv[0] == '9'){
            connectDNSServer(buffer_recv, buffer_send, argv[2]);
            break;
        }
        
        // processing DNS query
        if(buffer_recv[0] == '1') findIPAddress(buffer_recv, buffer_send, argv[2]);
        else findDomainName(buffer_recv, buffer_send, argv[2]);
        cout << buffer_send << endl;
        
        write(new_socket, buffer_send, BUFFER_SIZE);
        cout << "-------------------------------------------------------" << endl;
    }

    // closing connection
    cout << "Closing connection..." << endl;
    close(new_socket);
    close(socket_server);
    cout << "Connection closed" <<  endl;
    return 0;
}