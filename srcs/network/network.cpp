#include "../../include/main.hpp"

static int readMsg(int &clientSocket){

    char buffer[4096];

    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);                      // Read from the socket

    if (bytesReceived > 0)                                                                      
        return (logMsg('c', "[CLIENT]", std::string(buffer, bytesReceived)), bytesReceived);    // Succesfully read some bytes
    else if (bytesReceived == 0)                                                    
        return (logMsg('s', "[SERVER]", "Client disconnected"), 0);                             // Client disconnected gracefully
    else
        return (logMsg('e', "[SERVER]", "Something went wrong... "), -1);                       // Error, something went wrong
}

bool handleEvents(std::vector<struct pollfd> &clients){

    size_t i = 0;
    int pollResult = poll(&clients[0], clients.size(), -1);                                  // Check if any events happend
    if (pollResult >= 1)
    {
        while (i < clients.size())
        {
            if (clients[i].revents & POLLNVAL)                                                  // Invalid FD
            {
                logMsg('e', "[SERVER]", "Invalid file descriptor");             
                if (i == 0)                                                                     // If server, shutdown the program
                    return (false);

                clients.erase(clients.begin() + i);
                continue;
            }

            if ((clients[i].revents & POLLIN))
            {
                if (i == 0)                                                                     // 0 = Listening socket, tries to create a clientSocket and accept the connection
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
                }
                else 
                {
                    int readResult = readMsg(clients[i].fd);
                    if (readResult == 0 || readResult == -1){                                   // 0 = Disconnected gracefully | -1 = Something went wrong
                        close(clients[i].fd);
                        clients.erase(clients.begin() + i);
                        continue;
                    }
                }
            }

            if (clients[i].revents & (POLLHUP | POLLERR))
            {
                if (i == 0)
                    return (logMsg('e', "[SERVER]", "Client hangup or socket error"), false);

                logMsg('e', "[SERVER]", "Client hangup or socket error");
                close(clients[i].fd);
                clients.erase(clients.begin() + i);
                continue;
            }

            i++;
        }
    }
    else if (pollResult == 0)
        return (logMsg('e', "[SERVER]", "Timed out, no sockets ready. Shutting down server"), false);
    else if (pollResult == -1)
    {
        if (errno == EINTR)
        return true;

        logMsg('e', "[SERVER]", std::strerror(errno));
        return (false);
    }

    return (true);
}