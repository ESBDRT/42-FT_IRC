#include "../include/main.hpp"

int main(int argc, const char **argv){

    if (argc != 3)
        return (display_err("bad arguments: ./ircserv <port> <password>"), 1);

    int serverSocket;
    std::vector<struct pollfd> clients;

    if (init_server_socket(clients, serverSocket, argv[1]))
        return (1);

    while (true)
        handleEvents(clients);
   
    return (closeAll(clients), 0);
}
