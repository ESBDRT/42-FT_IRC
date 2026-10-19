#include "../../include/main.hpp"

// static int handleCommands(t_clientData &clientData){


// }

static int sendMsg(t_clientData &clientData, int &clientSocket, std::string msg){

    std::string formatMsg = "[";
    formatMsg += clientData.username;
    formatMsg += "] ";
    formatMsg += msg;
    formatMsg += "\n";

    ssize_t bytesSent = send(clientSocket, formatMsg.c_str(), formatMsg.size(), 0);
    
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

static bool subMsg(t_clientData &clientData, std::string &message)
{
    std::string::size_type pos = clientData.buffer.find("\r\n");

    if (pos == std::string::npos)
        return (false);
        
    message = clientData.buffer.substr(0, pos);
    clientData.buffer.erase(0, pos + 2);
    return (true);
}

static int readMsg(t_clientData &clientData, std::vector<struct pollfd> &clientsPolls, int &clientSocket){

    char tmp[4096];
    std::string msg;

    ssize_t bytesReceived = recv(clientSocket, &tmp, sizeof(tmp), 0);                      // Read from the socket

    if (bytesReceived > 0){

        clientData.buffer.append(tmp, bytesReceived);
        while (subMsg(clientData, msg)){
            broadcastMsg(clientData, clientsPolls, clientSocket, msg);     
            logMsg('c', clientData.username, msg);       
        }
        return (bytesReceived);                                                             // Succesfully read some bytes
    }                                                            
    else if (bytesReceived == 0)                                                    
        return (logMsg('s', clientData.username, "Client disconnected"), 0);                             // Client disconnected gracefully
    else
        return (logMsg('e', clientData.username, "Something went wrong... "), -1);                       // Error, something went wrong
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

                    struct pollfd newClientPoll;
                    newClientPoll.fd = clientSocket;
                    newClientPoll.events = POLLIN;
                    newClientPoll.revents = 0;
                    clients.push_back(newClientPoll);

                    t_clientData newClientData;
                    newClientData.poll = newClientPoll;
                    newClientData.buffer = "";
                    newClientData.username = "User";
                    newClientData.name = "User";
                    newClientData.hasName = false;
                    newClientData.hasUsername = false;
                    newClientData.passwordAccepted = false;
                    newClientData.isRegistered = false;
                    clientsData.push_back(newClientData);

                    logMsg('s', clientsData[0].username, "New client connected");
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