#include "../../include/main.hpp"

static int readMsg(int &clientSocket){

    char buffer[4096];

    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);          // Read from the socket

    if (bytesReceived > 0)                                                          // Succesfully read some bytes
    {    
        logMsg('c', "[CLIENT]", std::string(buffer, bytesReceived));
        return (bytesReceived);
    }
    else if (bytesReceived == 0)                                                    // Client disconnected
    {
        logMsg('s', "[SERVER]", "Client disconnected");
        return (0);
    }
    else 
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)              // Could not read but non fatal error
            return (-1);
        else if (errno == ECONNRESET || errno == ENOTCONN)                          // Connection issues
            return (logMsg('e', "[SERVER]", "Lost connection to client"), -2);
        else
            return (logMsg('e', "[SERVER]", std::strerror(errno)), -3);             // Something went wrong, fatal

    }
}

void handleEvents(std::vector<struct pollfd> &clients){

    size_t i = 0;
    int pollResult = poll(&clients[0], clients.size(), -1);                         // Check if any events happend
    if (pollResult >= 1)
    {
        while (i < clients.size())
        {
            if (!(clients[i].revents & POLLIN)){
                i++;
                continue;
            }
            
            if (clients[i].fd == clients[0].fd)                                     // Tries to create a clientSocket and accept the connection
            {
                int clientSocket = init_client_socket(clients[0].fd);
                if (clientSocket == -1){
                    i++;
                    continue;
                }

                struct pollfd newClient;
                newClient.fd = clientSocket;
                newClient.events = POLLIN;
                newClient.revents = 0;
                clients.push_back(newClient);

                logMsg('s', "[SERVER]", "New client connected");
                i++;
            }
            else 
            {
                int readResult = readMsg(clients[i].fd);
                if (readResult == 0 || readResult == -2 || readResult == -3){        // Disconnected gracefully - Connection issues - Fatal error
                    close(clients[i].fd);
                    clients.erase(clients.begin() + i);
                }
                else                                                                 // No issues
                    i++;
            }
        }
    }
}