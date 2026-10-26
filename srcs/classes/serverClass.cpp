#include "../../include/serverClass.hpp"
#include <unistd.h>

server::server(){};
server::~server(){
    closeAll();
}

void server::setPassword(std::string input){
    this->_password = input;
}

bool server::checkPassword(std::string input) const {
    return (input == this->_password ? true : false);
}

void server::closeAll(){
    for (size_t i = 0; i < _clients.size(); i++){
        if (_clients[i].poll.fd >= 0)
            close(_clients[i].poll.fd);
    }
    _clients.clear();
}

int server::getContactSocket(std::string contactUsername) const {

    for (size_t i = 1; i < this->_clients.size(); i++)
        if (this->_clients[i].username == contactUsername)
            return (this->_clients[i].poll.fd);
    return (-1);
}

std::vector<int> server::getChannelMembers(std::string channelName) const {

    for (size_t i = 0; i < this->_channels.size(); i++)
        if (this->_channels[i].name == channelName)
            return (this->_channels[i].members);

    return (std::vector<int>());
}

std::vector<t_channel> &server::getChannelsList(){
    return (this->_channels);
}

void server::addNewChannel(t_channel newChannel){
    this->_channels.push_back(newChannel);
}

t_channel &server::getChannelInfo(std::string channelName){

    for (size_t i = 0; i < this->_channels.size(); i++)
        if (this->_channels[i].name == channelName)
            return (this->_channels[i]);

    static t_channel emptyChannel;
    emptyChannel.name = "";
    emptyChannel.members.clear();
    emptyChannel.operators.clear();
    return (emptyChannel);
}   

bool server::isOperator(t_channel &channel, int &clientSocket) const {

    for (size_t i = 0; i < channel.operators.size(); i++)
        if (clientSocket == channel.operators[i])
            return (true);
    
    return (false);
}