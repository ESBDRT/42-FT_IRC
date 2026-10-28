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
