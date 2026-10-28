#include "../../include/main.hpp"

// THIS FUNCTION IS USED ONLY FOR LOGGING ON THE SENDING CLIENT ITSELF
int sendMsg(ClientData &clientData, int &clientSocket, std::string msg){

    std::string formatMsg = "[" + clientData.username + "]" + msg + "\n";

    ssize_t bytesSent = send(clientSocket, formatMsg.c_str(), formatMsg.size(), 0);
    
    return (bytesSent == -1 ? 1 : 0); 
}

// Send a IRC formatted msg
int sendPrivmsg(ClientData &clientData, int &clientSocket, const std::string &target, const std::string &msg){

    std::string formatMsg = ":" + clientData.username + "!" + clientData.user + "@localhost PRIVMSG " + target + " :" + msg + "\r\n";

    ssize_t bytesSent = send(clientSocket, formatMsg.c_str(), formatMsg.size(), 0);

    return (bytesSent == static_cast<ssize_t>(formatMsg.size()) ? 0 : 1);
}

// Broadcast the message to every user in a channel except sender
void broadcastMsg(ClientData &clientData, std::vector<int> channelMembersSockets, const std::string &target, const std::string &msg){

    for (size_t i = 0; i < channelMembersSockets.size(); ++i){

        if (channelMembersSockets[i] == clientData.poll.fd)
            continue;

        if (sendPrivmsg(clientData, channelMembersSockets[i], target, msg))
            logMsg('e', std::string(clientData.username + " failed to send channel message"), target);
    }
}

// Search for a full irc formatted message, return false if not found, else extract the message from the buffer 
static bool subMsg(ClientData &clientData, std::string &message)
{
    std::string::size_type pos = clientData.buffer.find("\r\n");

    if (pos == std::string::npos)
        return (false);
        
    message = clientData.buffer.substr(0, pos);
    clientData.buffer.erase(0, pos + 2);
    return (true);
}

// Read data from the socket
static int readMsg(server &server, ClientData &clientData, int &clientSocket){

    char tmp[4096];
    std::string msg;

    // Read from the socket
    ssize_t bytesReceived = recv(clientSocket, &tmp, sizeof(tmp), 0);

    // Succesfully read some bytes
    if (bytesReceived > 0){

        clientData.buffer.append(tmp, bytesReceived);
        while (subMsg(clientData, msg))
            handleCommands(server, clientData, msg);

        return (bytesReceived);
    }                                

    // Client disconnected gracefully
    else if (bytesReceived == 0)                                                    
        return (logMsg('i', "Client disconnected:", clientData.username), 0);
    
    // Error, something went wrong
    else
        return (logMsg('e', "Something went wrong..", ""), -1);
}

bool server::run(){

    // Build a vector of the clients pollfds
    std::vector<struct pollfd> pollFds;
    for (size_t j = 0; j < _clients.size(); j++)
        pollFds.push_back(_clients[j].poll);

    // Check if any events happend
    int pollResult = poll(&pollFds[0], pollFds.size(), -1);
    if (pollResult >= 1)
    {
        for (size_t j = 0; j < _clients.size(); j++)
            _clients[j].poll.revents = pollFds[j].revents;

        size_t i = 0;
        while (i < _clients.size())
        {
            // POLLNVAL = Invalid FD
            if (_clients[i].poll.revents & POLLNVAL)
            {
                logMsg('e', "[SERVER]", "Invalid file descriptor");

                // If server, shutdown the program            
                if (i == 0)                                                                     
                    return (false);
                
                // Delete the client (pollFds is rebuilt on every run() call, no need to remove it there)
                removeSocketFromChannels(*this, _clients[i].poll.fd);
                _clients.erase(_clients.begin() + i);
                continue;
            }

            // POLLIN = Data received
            if ((_clients[i].poll.revents & POLLIN))
            {
                // 0 = Listening socket, tries to create a clientSocket and accept the connection
                if (i == 0)                                                                     
                {
                    int clientSocket = init_client_socket(_clients[0].poll.fd);
                    if (clientSocket == -1){
                        i++;
                        continue;
                    }

                    // Create new poll structure for the client
                    struct pollfd newClientPoll;
                    newClientPoll.fd = clientSocket;
                    newClientPoll.events = POLLIN;
                    newClientPoll.revents = 0;

                    // Create new clientData structure with default settings
                    ClientData newClientData;
                    newClientData.poll = newClientPoll;
                    newClientData.buffer = "";
                    newClientData.username = "Default";
                    newClientData.name = "Default";
                    newClientData.user = "Default";
                    newClientData.hasName = false;
                    newClientData.hasUsername = false;
                    newClientData.passwordAccepted = false;
                    newClientData.isRegistered = false;
                    _clients.push_back(newClientData);

                    logMsg('i', "New client connected:", newClientData.username);
                }

                // If a client has received data
                else 
                {
                    // Read the data
                    int readResult = readMsg(*this, _clients[i], _clients[i].poll.fd);

                    // 0 = Disconnected gracefully | -1 = Something went wrong
                    if (readResult == 0 || readResult == -1){
                        
                        // Remove from channels, close socket and remove client
                        removeSocketFromChannels(*this, _clients[i].poll.fd);
                        close(_clients[i].poll.fd);
                        _clients.erase(_clients.begin() + i);
                        continue;
                    }
                }
            }

            // POLLHUP | POLLERR = Disconnetion or errors
            if (_clients[i].poll.revents & (POLLHUP | POLLERR))
            {
                // If server failure, shutdown
                if (i == 0)
                    return (logMsg('e', "[SERVER]", "Client hangup or socket error"), false);

                // Log errors and close + remove client
                logMsg('e', "[SERVER]", "Client hangup or socket error");
                removeSocketFromChannels(*this, _clients[i].poll.fd);
                close(_clients[i].poll.fd);
                _clients.erase(_clients.begin() + i);
                continue;
            }

            i++;
        }
    }

    // If poll failed
    else if (pollResult == -1)
    {
        // Interrupted by a signal, main checks g_shutdown to know if the server must stop
        if (errno == EINTR)
            return true;

        logMsg('e', "[SERVER]", std::strerror(errno));
        return (false);
    }

    return (true);
}