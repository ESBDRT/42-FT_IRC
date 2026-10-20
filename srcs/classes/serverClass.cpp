#include "../../include/serverClass.hpp"

server::server(){};
server::~server(){};

void server::setPassword(std::string input){
    this->_password = input;
}

bool server::checkPassword(std::string input){
    return (input == this->_password ? true : false);
}
