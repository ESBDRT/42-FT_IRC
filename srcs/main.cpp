#include "../include/main.hpp"

int readMsg(int &clientSocket)
{
    char buffer[512];

    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived > 0)
    {
        std::cout.write(buffer, bytesReceived);
        return (0);
    }
    else if (bytesReceived == 0)
    {
        close(clientSocket);
        logMsg('s', "Client disconnected");
        return (1);
    }
    else
        //TODO: Verify error handling here, could cause issues with this return value
        return (logMsg('e', std::strerror(errno)), -1);
        
}

int main(int argc, const char **argv){

    if (argc != 3)
        return (display_err("bad arguments: ./ircserv <port> <password>"), 1);

    int serverSocket;
    std::vector<struct pollfd> clients;

    if (init_server_socket(clients, serverSocket, argv[1]))
        return (1);

    while (true)
    {
        int pollResult = poll(&clients[0], clients.size(), -1);
        if (pollResult >= 1)
        {
            for (size_t i = 0; i < clients.size(); i++)
            {
                if (!(clients[i].revents & POLLIN))
                    continue;
                
                if (clients[i].fd == serverSocket)
                {
                    int clientSocket = init_client_socket(serverSocket);
                    if (clientSocket == -1)
                        continue;

                    struct pollfd newClient;
                    newClient.fd = clientSocket;
                    newClient.events = POLLIN;
                    newClient.revents = 0;

                    clients.push_back(newClient);
                    logMsg('s', "New client connected");
                }
                else
                    readMsg(clients[i].fd);
            }
            
        }
    }
   
    return (close(serverSocket), 0);
}
