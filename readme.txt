Usage:

g++ -o DNS DNS.cpp
./DNS <DNS_port>

g++ -o proxy proxy.cpp
./proxy <proxy_port> <DNS_ip_address> <DNS_port>

g++ -o client client.cpp
./client <proxy_ip_address> <proxy_port>

Note: Compile and execute all the three programs in three different terminal tabs
Use IP address which is shown in "ifconfig" command