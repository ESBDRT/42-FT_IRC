#include "../include/main.hpp"

int main(int argc, const char **argv){

    if (argc != 3)
        return (display_err("bad arguments: ./ircserv <port> <password>"), 1);

    server serverObj;
    serverObj.setPassword(std::string(argv[2]));

    int serverSocket;
    std::vector<struct pollfd> clientsPolls;
    std::vector<t_clientData> clientsData;

    if (init_server_socket(clientsData, clientsPolls, serverSocket, argv[1]))
        return (1);

    while (true)
        if (!handleEvents(serverObj, clientsData, clientsPolls))
            break;
   
    return (closeAll(clientsPolls), 0);
}
