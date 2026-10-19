#include "../../include/main.hpp"

static int sendMsg(t_clientData &clientData, int &clientSocket, std::string msg){

    std::string finalmsg = "[";
    finalmsg += clientData.nickname;
    finalmsg += "] ";
    finalmsg += msg;

    ssize_t bytesSent = send(clientSocket, finalmsg.c_str(), finalmsg.size(), 0);
    
    return (bytesSent == -1 ? 1 : 0); 
}

static int broadcastMsg(t_clientData &clientData, std::vector<struct pollfd> &clients, int &clientSocket, std::string msg){

    for (size_t i = 1; i < clients.size(); i++)
    {
        if (clients[i].fd == clientSocket)
            continue;
            
        sendMsg(clientData, clients[i].fd, msg);
    }
    
    return (0);
}

static int readMsg(t_clientData &clientData, std::vector<struct pollfd> &clients, int &clientSocket){

    char buffer[4096];

    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);                      // Read from the socket

    if (bytesReceived > 0){
        broadcastMsg(clientData, clients, clientSocket, std::string(buffer, bytesReceived));
        return (logMsg('c', clientData.nickname, std::string(buffer, bytesReceived)), bytesReceived);    // Succesfully read some bytes
    }                                                            
    else if (bytesReceived == 0)                                                    
        return (logMsg('s', clientData.nickname, "Client disconnected"), 0);                             // Client disconnected gracefully
    else
        return (logMsg('e', clientData.nickname, "Something went wrong... "), -1);                       // Error, something went wrong
}

bool handleEvents(std::vector<t_clientData> &clientsData, std::vector<struct pollfd> &clients){

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
                clientsData.erase(clientsData.begin() + i);
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

                    t_clientData newClientData;
                    newClientData.poll = newClient;
                    newClientData.buffer = "";
                    newClientData.nickname = "DEFAULT NAME";
                    clientsData.push_back(newClientData);

                    logMsg('s', clientsData[0].nickname, "New client connected");
                }
                else 
                {
                    int readResult = readMsg(clientsData[i], clients, clients[i].fd);
                    if (readResult == 0 || readResult == -1){                                   // 0 = Disconnected gracefully | -1 = Something went wrong
                        close(clients[i].fd);
                        clients.erase(clients.begin() + i);
                        clientsData.erase(clientsData.begin() + i);
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
                clientsData.erase(clientsData.begin() + i);
                continue;
            }

            i++;
        }
    }
    else if (pollResult == -1)
    {
        if (errno == EINTR)
        return true;

        logMsg('e', "[SERVER]", std::strerror(errno));
        return (false);
    }

    return (true);
}