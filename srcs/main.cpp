#include "../include/main.hpp"

void test(int &clientSocket)
{
    char buffer[512];

    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived > 0)
    {
        std::cout.write(buffer, bytesReceived);
        std::cout << std::endl;
    }
    else if (bytesReceived == 0)
        logMsg('s', "Client disconnected");
    else
        logMsg('e', std::strerror(errno));
}

int main(int argc, const char **argv)
{
    int serverSocket;
    std::vector<struct pollfd> clients;

    if (argc != 3)
        return (display_err("bad arguments: ./ircserv <port> <password>"), 1);

    if (init_server_socket(clients, serverSocket, argv[1]))
        return (1);

    while (true)
    {
        struct pollfd newClient;
        
        newClient.fd = init_client_socket(serverSocket);
        if (newClient.fd == -1)
            continue;
        
        newClient.events = POLLIN;
        newClient.revents = 0;
        clients.push_back(newClient);

        logMsg('s', "New client connected");
    }

    // There will be a function to close all open sockets
   
    return (close(serverSocket), 0);
}
