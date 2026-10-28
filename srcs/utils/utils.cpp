#include "../../include/main.hpp"

// Display time in european format
static std::string displayTime(){

    char time[15];

    std::time_t now = std::time(NULL);
    std::tm* localTime = std::localtime(&now);

    if (!localTime || std::strftime(time, sizeof(time), "[%H:%M:%S]: ", localTime) == 0)
        return "";

    return time;
}

// Log the message using colors depending on the level
void logMsg(char lvl, std::string msg, std::string arg){
    
    const char *CLR;
    const char *LEVEL;

    switch (lvl) {
        case 'i':
            CLR = CLR_BLUE;
            LEVEL = "INFO";
            break;
        case 'w':
            CLR = CLR_YELLOW;
            LEVEL = "WARNING";
            break;
        case 'e':
            CLR = CLR_RED;
            LEVEL = "ERROR";
            break;
        case 'c':
            CLR = CLR_GREEN;
            LEVEL = "CLIENT";
            break;
        default:
            CLR = CLR_RST;
            LEVEL = "LOG";
            break;
    }

    std::ostream& out = (lvl == 'e') ? std::cerr : std::cout;

    out << "[" << CLR << LEVEL << CLR_RST << "]" << displayTime() << msg << CLR_RST;

    if (!arg.empty())
        out << " " << CLR << arg << CLR_RST;

    out << std::endl;
}

// Log the err message in the STDERR
void display_err(std::string err){
    std::cerr << CLR_RED << "[ERROR]: " <<  CLR_RST << err << CLR_RST << std::endl;
}

// Remove every occurence of the socket from the provided vector
static void removeSocketFromVector(std::vector<int> &sockets, int socket){

    size_t i = 0;
    while (i < sockets.size())
    {
        if (sockets[i] == socket)
            sockets.erase(sockets.begin() + i);
        else
            i++;
    }
}

// Remove the socket from the members, operators and invites of the channel at the provided index
// The channel is deleted if it has no members left, returns true in that case
static bool removeSocketFromChannelAt(std::vector<Channel> &channels, size_t index, int socket){

    removeSocketFromVector(channels[index].members, socket);
    removeSocketFromVector(channels[index].operators, socket);
    removeSocketFromVector(channels[index].invited, socket);

    if (channels[index].members.empty()){
        logMsg('i', "Channel deleted, no members left:", channels[index].name);
        channels.erase(channels.begin() + index);
        return (true);
    }
    return (false);
}

// Remove the socket from the members, operators and invites of every channel, must be called before the socket is closed
// Channels left without any member are deleted
void removeSocketFromChannels(server &server, int socket){

    std::vector<Channel> &channels = server.getChannelsList();

    size_t i = 0;
    while (i < channels.size())
        if (!removeSocketFromChannelAt(channels, i, socket))
            i++;
}

// Every socket sharing at least one channel with the provided socket, each listed once, the socket itself excluded
std::vector<int> getChannelPeers(server &server, int socket){

    std::vector<Channel> &channels = server.getChannelsList();
    std::vector<int> peers;

    for (size_t i = 0; i < channels.size(); i++)
    {
        const std::vector<int> &members = channels[i].members;
        if (std::find(members.begin(), members.end(), socket) == members.end())
            continue;

        for (size_t j = 0; j < members.size(); j++)
            if (members[j] != socket && std::find(peers.begin(), peers.end(), members[j]) == peers.end())
                peers.push_back(members[j]);
    }
    return (peers);
}

// Announce the departure of the client to every user sharing a channel with him, then remove him from every channel
// Calling it again does nothing since the client isn't in any channel anymore
void quitChannels(server &server, ClientData &clientData, const std::string &reason){

    if (clientData.isRegistered){
        const std::string quitMsg = ":" + clientData.username + "!" + clientData.user + "@localhost QUIT :" + reason;
        std::vector<int> peers = getChannelPeers(server, clientData.poll.fd);
        for (size_t i = 0; i < peers.size(); i++)
            server.queueMsg(peers[i], quitMsg);
    }
    removeSocketFromChannels(server, clientData.poll.fd);
}

// Remove the socket from a single channel (KICK), the channel is deleted if it has no members left
// Any reference to that channel must not be used after this call
void removeSocketFromChannel(server &server, const std::string &channelName, int socket){

    std::vector<Channel> &channels = server.getChannelsList();

    for (size_t i = 0; i < channels.size(); i++){
        if (channels[i].name == channelName){
            removeSocketFromChannelAt(channels, i, socket);
            return ;
        }
    }
}
