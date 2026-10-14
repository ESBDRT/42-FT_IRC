#include "../../include/main.hpp"

static int readMsg(int &clientSocket)
{
    char buffer[4096];

    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    if (bytesReceived > 0)
    {    
        logMsg('c', "[CLIENT]", std::string(buffer, bytesReceived));
        return (bytesReceived);
    }
    else if (bytesReceived == 0)
    {
        logMsg('s', "[SERVER]", "Client disconnected");
        return (0);
    }
    else
        //TODO: Verify error handling here, could cause issues with this return value
        return (logMsg('e', "[SERVER]", std::strerror(errno)), -1);  
}

void handleEvents(std::vector<struct pollfd> &clients){

    int pollResult = poll(&clients[0], clients.size(), -1);
    if (pollResult >= 1)
    {
        for (size_t i = 0; i < clients.size(); i++)
        {
            if (!(clients[i].revents & POLLIN))
                continue;
            
            if (clients[i].fd == clients[0].fd)
            {
                int clientSocket = init_client_socket(clients[0].fd);
                if (clientSocket == -1)
                    continue;
                struct pollfd newClient;
                newClient.fd = clientSocket;
                newClient.events = POLLIN;
                newClient.revents = 0;
                clients.push_back(newClient);
                logMsg('s', "[SERVER]", "New client connected");
            }
            else 
            {
                int readResult = readMsg(clients[i].fd);
                if (readResult == 0)                                        // Disconnected gracefully
                {
                    close(clients[i].fd);
                    clients.erase(clients.begin() + i);
                }
                else if (readResult == 1)                                   //TODO: Something went wrong
                {
                    continue;
                }
                else                                                        // No issues
                    continue ;
            }
        }
    }
}