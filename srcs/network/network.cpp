#include "../../include/main.hpp"

// Queue an IRC line for the client, it is sent by run() once poll() reports the socket as writable
void queueMsg(ClientData &clientData, const std::string &msg){

    // Nothing is queued anymore for a client that is being disconnected
    if (clientData.closing)
        return ;

    clientData.outBuffer += msg + "\r\n";

    // The client doesn't read fast enough, disconnect him instead of using more and more memory (SendQ exceeded)
    if (clientData.outBuffer.size() > MAX_SENDQ){
        logMsg('e', "Send queue exceeded, disconnecting client:", clientData.username);
        clientData.outBuffer.clear();
        clientData.closing = true;
        clientData.quitReason = "Send queue exceeded";
    }
}

// Queue an IRC line for the client using that socket, if there is one
void server::queueMsg(int clientSocket, const std::string &msg){

    ClientData *client = getClient(clientSocket);
    if (client)
        ::queueMsg(*client, msg);
}

// THIS FUNCTION IS USED ONLY FOR LOGGING ON THE SENDING CLIENT ITSELF
void sendMsg(ClientData &clientData, std::string msg){
    queueMsg(clientData, "[" + clientData.username + "]" + msg);
}

// Queue a IRC formatted PRIVMSG from the sender to the provided socket
void sendPrivmsg(server &server, ClientData &clientData, int clientSocket, const std::string &target, const std::string &msg){
    server.queueMsg(clientSocket, ":" + clientData.username + "!" + clientData.user + "@localhost PRIVMSG " + target + " :" + msg);
}

// Broadcast the message to every user in a channel except sender
void broadcastMsg(server &server, ClientData &clientData, const std::vector<int> &channelMembersSockets, const std::string &target, const std::string &msg){

    for (size_t i = 0; i < channelMembersSockets.size(); ++i){

        if (channelMembersSockets[i] == clientData.poll.fd)
            continue;

        sendPrivmsg(server, clientData, channelMembersSockets[i], target, msg);
    }
}

// Tell the client its line was dropped (417 == ERR_INPUTTOOLONG)
static void sendInputTooLong(ClientData &clientData){

    const std::string target = clientData.isRegistered ? clientData.username : "*";
    queueMsg(clientData, ":localhost 417 " + target + " :Input line was too long");
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
        while (!clientData.closing && subMsg(clientData, msg))
        {
            // End of a line that was too long, its beginning has arleady been dropped
            if (clientData.discardUntilEol){
                clientData.discardUntilEol = false;
                continue;
            }

            // Lines are limited to 512 bytes, \r\n included
            if (msg.size() + 2 > IRC_MAX_LINE){
                sendInputTooLong(clientData);
                continue;
            }

            handleCommands(server, clientData, msg);
        }

        // No \r\n in the first 512 bytes, the line is too long: drop what has been received, and the rest of the line until the next \r\n
        // This keeps the buffer small whatever the client sends
        if (clientData.buffer.size() >= IRC_MAX_LINE){

            if (!clientData.discardUntilEol)
                sendInputTooLong(clientData);
            clientData.discardUntilEol = true;

            // Keep a trailing \r, it can be the first half of a \r\n split between two recv
            const bool keepCR = clientData.buffer[clientData.buffer.size() - 1] == '\r';
            clientData.buffer = keepCR ? "\r" : "";
        }

        return (bytesReceived);
    }

    // Client disconnected gracefully
    else if (bytesReceived == 0)
        return (logMsg('i', "Client disconnected:", clientData.username), 0);

    // Error, something went wrong
    else
        return (logMsg('e', "Something went wrong..", ""), -1);
}

// Announce the departure to the client channels, remove him from every channel, close its socket and remove it from the clients list
void server::disconnectClient(size_t index, const std::string &reason){

    quitChannels(*this, _clients[index], reason);
    close(_clients[index].poll.fd);
    _clients.erase(_clients.begin() + index);
}

// Disconnect the clients flagged as closing once they have nothing left to send
// Called at the end of run(), so a client flagged while processing events doesn't wait for the next poll() event
void server::disconnectClosingClients(){

    size_t i = 1;
    while (i < _clients.size()){
        if (_clients[i].closing && _clients[i].outBuffer.empty())
            disconnectClient(i, _clients[i].quitReason.empty() ? "Connection closed" : _clients[i].quitReason);
        else
            i++;
    }
}

bool server::run(){

    // Build a vector of the clients pollfds
    // POLLOUT is only requested when there is something to send, otherwise poll() would return immediately
    // Nothing is read anymore from a client that is being disconnected
    std::vector<struct pollfd> pollFds;
    for (size_t j = 0; j < _clients.size(); j++){
        struct pollfd clientPoll = _clients[j].poll;
        clientPoll.events = _clients[j].closing ? 0 : POLLIN;
        if (!_clients[j].outBuffer.empty())
            clientPoll.events |= POLLOUT;
        clientPoll.revents = 0;
        pollFds.push_back(clientPoll);
    }

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
                disconnectClient(i, "Connection error");
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
                        disconnectClient(i, readResult == 0 ? "Connection closed" : "Read error");
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
                disconnectClient(i, "Connection closed");
                continue;
            }

            // POLLOUT = The socket can be written, send as much of the queued data as possible
            // Whatever has not been sent stays in outBuffer until the next POLLOUT
            if ((_clients[i].poll.revents & POLLOUT) && !_clients[i].outBuffer.empty())
            {
                ssize_t bytesSent = send(_clients[i].poll.fd, _clients[i].outBuffer.data(), _clients[i].outBuffer.size(), 0);
                if (bytesSent == -1){
                    logMsg('e', "Failed to send data, disconnecting client:", _clients[i].username);
                    disconnectClient(i, "Write error");
                    continue;
                }
                _clients[i].outBuffer.erase(0, bytesSent);
            }

            i++;
        }

        disconnectClosingClients();
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
