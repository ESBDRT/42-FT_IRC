#include "../../include/serverClass.hpp"
#include <unistd.h>

server::server(){};
server::~server(){
    closeAll();
}

void server::setPassword(std::string input){
    this->_password = input;
}

bool server::checkPassword(std::string input){
    return (input == this->_password ? true : false);
}

void server::closeAll(){
    for (size_t i = 0; i < _clients.size(); i++){
        if (_clients[i].poll.fd >= 0)
            close(_clients[i].poll.fd);
    }
    _clients.clear();
}

int server::getContactSocket(std::string contactName){

    for (size_t i = 1; i < this->_clients.size(); i++)
        if (this->_clients[i].name == contactName)
            return (this->_clients[i].poll.fd);
    
    return (-1);
}
