#include "../include/main.hpp"

int main(int argc, const char **argv){

    if (argc != 3)
        return (display_err("bad arguments: ./ircserv <port> <password>"), 1);

    int serverSocket;
    std::vector<struct pollfd> clients;
    std::vector<t_clientData> clientsData;

    if (init_server_socket(clientsData, clients, serverSocket, argv[1]))
        return (1);

    while (true)
        if (!handleEvents(clientsData, clients))
            break;
   
    return (closeAll(clients), 0);
}
