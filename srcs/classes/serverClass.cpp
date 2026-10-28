#include "../../include/serverClass.hpp"
#include <unistd.h>

ClientData::ClientData()
    : buffer(), name(), username(), user(), hasName(false), hasUsername(false),
      passwordAccepted(false), isRegistered(false){
    poll.fd = -1;
    poll.events = 0;
    poll.revents = 0;
}

ClientData::ClientData(const ClientData &other)
    : poll(other.poll), buffer(other.buffer), name(other.name),
      username(other.username), user(other.user), hasName(other.hasName),
      hasUsername(other.hasUsername), passwordAccepted(other.passwordAccepted),
      isRegistered(other.isRegistered){
}

ClientData &ClientData::operator=(const ClientData &other){
    if (this != &other){
        poll = other.poll;
        buffer = other.buffer;
        name = other.name;
        username = other.username;
        user = other.user;
        hasName = other.hasName;
        hasUsername = other.hasUsername;
        passwordAccepted = other.passwordAccepted;
        isRegistered = other.isRegistered;
    }
    return (*this);
}

ClientData::~ClientData(){
}

Channel::Channel()
    : members(), operators(), name(), topic(), password(),
      isPasswordLocked(false), restrictedTopic(false), inviteOnly(false),
      userLimit(false), maxUsers(0), currentUsers(0){
}

Channel::Channel(const Channel &other)
    : members(other.members), operators(other.operators), name(other.name),
      topic(other.topic), password(other.password),
      isPasswordLocked(other.isPasswordLocked),
      restrictedTopic(other.restrictedTopic), inviteOnly(other.inviteOnly),
      userLimit(other.userLimit), maxUsers(other.maxUsers),
      currentUsers(other.currentUsers){
}

Channel &Channel::operator=(const Channel &other){
    if (this != &other){
        members = other.members;
        operators = other.operators;
        name = other.name;
        topic = other.topic;
        password = other.password;
        isPasswordLocked = other.isPasswordLocked;
        restrictedTopic = other.restrictedTopic;
        inviteOnly = other.inviteOnly;
        userLimit = other.userLimit;
        maxUsers = other.maxUsers;
        currentUsers = other.currentUsers;
    }
    return (*this);
}

Channel::~Channel(){
}

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

std::string server::getNickname(int clientSocket) const {

    for (size_t i = 1; i < this->_clients.size(); i++)
        if (this->_clients[i].poll.fd == clientSocket)
            return (this->_clients[i].username);
    return ("");
}

std::vector<int> server::getChannelMembers(std::string channelName) const {

    for (size_t i = 0; i < this->_channels.size(); i++)
        if (this->_channels[i].name == channelName)
            return (this->_channels[i].members);

    return (std::vector<int>());
}

std::vector<Channel> &server::getChannelsList(){
    return (this->_channels);
}

void server::addNewChannel(Channel newChannel){
    this->_channels.push_back(newChannel);
}

Channel &server::getChannelInfo(std::string channelName){

    for (size_t i = 0; i < this->_channels.size(); i++)
        if (this->_channels[i].name == channelName)
            return (this->_channels[i]);

    static Channel emptyChannel;
    emptyChannel.name = "";
    emptyChannel.members.clear();
    emptyChannel.operators.clear();
    return (emptyChannel);
}   

bool server::isOperator(Channel &channel, int &clientSocket) const {

    for (size_t i = 0; i < channel.operators.size(); i++)
        if (clientSocket == channel.operators[i])
            return (true);
    
    return (false);
}