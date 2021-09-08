#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h> 
#define BUFFER_SIZE 1024
#define PORT 12345
using namespace std;

int printError(string errorMessage){
    cout << errorMessage << endl;
    exit(-1);
}

int main(int argc, char* argv[]){
    if(argc != 3) printError("Invalid number of arguments!");

    // creating new socket for communication
    cout << "Creating new socket..." << endl;
    int socket_client = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_client == -1) printError("Error creating socket!");
    else cout << "Socket created successfully." << endl;

    // configuring socket to reuse IP address and port number
    int opt = 1;
    if(setsockopt(socket_client, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) 
        printError("Error in configuring socket!");

    // configuring socket pair attributes
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    // if(inet_pton(AF_INET, argv[1], &server_address) == -1) printError("Invalid IP address!");
    server_address.sin_addr.s_addr = inet_addr(argv[1]);
    server_address.sin_port = htons(atoi(argv[2]));

    // establishing connection with the proxy server
    cout << "Connecting to the server..." <<  endl;
    int conn_status = connect(socket_client, (struct sockaddr *)&server_address, sizeof(server_address));
    if(conn_status == -1) printError("Error connecting to the server!");
    else cout << "Connected to the server." << endl;
    cout << "------------------------------------------------------" << endl;

    while(true){
        int type;
        cout << "\n 1. Get IP address from domain name" << endl;
        cout << " 2. Get domain name from IP address" << endl;
        cout << " 9. Exit" << endl;
        cout << "Enter your choice: ";
        cin >> type;

        if(type == 9){
            write(socket_client, "9$Close connection", BUFFER_SIZE);
            break;
        }

        // creating DNS query
        char buffer_request[BUFFER_SIZE] = {0};
        if(type == 1) cout << "Enter domain name: ";
        if(type == 2) cout << "Enter IP address: ";
        buffer_request[0] = '0' + type;
        buffer_request[1] = '$';
        cin >> buffer_request+2;

        write(socket_client, buffer_request, BUFFER_SIZE);

        char buffer_reply[BUFFER_SIZE] = {0};
        read(socket_client, buffer_reply, BUFFER_SIZE);

        if(buffer_reply[0] == '3'){
            if(type == 1) cout << "The IP address is: ";
            else cout << "The domain name is: ";
        }
        cout << buffer_reply+2 << endl;
        cout << "------------------------------------------------------" << endl;
    }

    // closing connection
    cout << "\nClosing connection..." << endl;
    close(socket_client);
    cout << "Connection closed" <<  endl;
    return 0;
}