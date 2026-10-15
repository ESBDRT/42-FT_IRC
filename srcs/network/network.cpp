#include "../../include/main.hpp"

static int readMsg(int &clientSocket){

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
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
            return (-1);
        else if (errno == ECONNRESET || errno == ENOTCONN)
            return (logMsg('e', "[SERVER]", "Lost connection to client"), -2);
        else
            return (logMsg('e', "[SERVER]", std::strerror(errno)), -3);  

    }
}

void handleEvents(std::vector<struct pollfd> &clients){

    size_t i = 0;
    int pollResult = poll(&clients[0], clients.size(), -1);
    if (pollResult >= 1)
    {
        while (i < clients.size())
        {
            if (!(clients[i].revents & POLLIN)){
                i++;
                continue;
            }
            
            if (clients[i].fd == clients[0].fd)
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
                if (readResult == 0 || readResult == -2 || readResult == -3){                                       // Disconnected gracefully
                    close(clients[i].fd);
                    clients.erase(clients.begin() + i);
                }
                else                                                        // No issues
                    i++;
            }
        }
    }
}