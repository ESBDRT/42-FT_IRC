#include "../../include/main.hpp"

int sendMsg(ClientData &clientData, int &clientSocket, std::string msg){

    std::string formatMsg = "[" + clientData.username + "]" + msg + "\n";

    ssize_t bytesSent = send(clientSocket, formatMsg.c_str(), formatMsg.size(), 0);
    
    return (bytesSent == -1 ? 1 : 0); 
}

void broadcastMsg(ClientData &clientData, std::vector<int> channelMembersSockets, std::string msg){

    for (size_t i = 0; i < channelMembersSockets.size(); i++)
    {
        if (sendMsg(clientData, channelMembersSockets[i], msg))
            logMsg('e', std::string (clientData.username + " failed to send message, something went wrong.."), "");
    }
}

static bool subMsg(ClientData &clientData, std::string &message)
{
    std::string::size_type pos = clientData.buffer.find("\r\n");

    if (pos == std::string::npos)
        return (false);
        
    message = clientData.buffer.substr(0, pos);
    clientData.buffer.erase(0, pos + 2);
    return (true);
}

static int readMsg(server &server, ClientData &clientData, int &clientSocket){

    char tmp[4096];
    std::string msg;

    ssize_t bytesReceived = recv(clientSocket, &tmp, sizeof(tmp), 0);                      // Read from the socket

    if (bytesReceived > 0){

        clientData.buffer.append(tmp, bytesReceived);
        while (subMsg(clientData, msg))
            handleCommands(server, clientData, msg);

        return (bytesReceived);                                                             // Succesfully read some bytes
    }                                                            
    else if (bytesReceived == 0)                                                    
        return (logMsg('i', "Client disconnected:", clientData.username), 0);               // Client disconnected gracefully
    else
        return (logMsg('e', "Something went wrong..", ""), -1);                             // Error, something went wrong
}

bool server::run(){

    size_t i = 0;
    std::vector<struct pollfd> pollFds;
    for (size_t j = 0; j < _clients.size(); j++)
        pollFds.push_back(_clients[j].poll);

    int pollResult = poll(&pollFds[0], pollFds.size(), -1);                                 // Check if any events happend
    if (pollResult >= 1)
    {
        for (size_t j = 0; j < _clients.size(); j++)
            _clients[j].poll.revents = pollFds[j].revents;

        while (i < _clients.size())
        {
            if (_clients[i].poll.revents & POLLNVAL)                                             // Invalid FD
            {
                logMsg('e', "[SERVER]", "Invalid file descriptor");             
                if (i == 0)                                                                     // If server, shutdown the program
                    return (false);

                _clients.erase(_clients.begin() + i);
                continue;
            }

            if ((_clients[i].poll.revents & POLLIN))
            {
                if (i == 0)                                                                     // 0 = Listening socket, tries to create a clientSocket and accept the connection
                {
                    int clientSocket = init_client_socket(_clients[0].poll.fd);
                    if (clientSocket == -1){
                        i++;
                        continue;
                    }

                    struct pollfd newClientPoll;
                    newClientPoll.fd = clientSocket;
                    newClientPoll.events = POLLIN;
                    newClientPoll.revents = 0;

                    ClientData newClientData;
                    newClientData.poll = newClientPoll;
                    newClientData.buffer = "";
                    newClientData.username = "Default";
                    newClientData.name = "Default";
                    newClientData.hasName = false;
                    newClientData.hasUsername = false;
                    newClientData.passwordAccepted = false;
                    newClientData.isRegistered = false;
                    _clients.push_back(newClientData);

                    logMsg('i', "New client connected:", newClientData.username);
                    sendMsg(newClientData, newClientData.poll.fd, std::string(" Authentificate yourself using /quote USER <username> * 0 :<name> AND /quote PASS <password>"));
                }
                else 
                {
                    int readResult = readMsg(*this, _clients[i], _clients[i].poll.fd);
                    if (readResult == 0 || readResult == -1){                                       // 0 = Disconnected gracefully | -1 = Something went wrong
                        close(_clients[i].poll.fd);
                        _clients.erase(_clients.begin() + i);
                        continue;
                    }
                }
            }

            if (_clients[i].poll.revents & (POLLHUP | POLLERR))
            {
                if (i == 0)
                    return (logMsg('e', "[SERVER]", "Client hangup or socket error"), false);

                logMsg('e', "[SERVER]", "Client hangup or socket error");
                close(_clients[i].poll.fd);
                _clients.erase(_clients.begin() + i);
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