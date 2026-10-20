#pragma once

// ----------------------------------|INCLUDES|---------------------------------- //

#include <iostream>
#include <sstream>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <climits>
#include <cstdlib>
#include <cerrno>
#include <ctime>
#include <unistd.h>
#include <vector>
#include <poll.h>

// ----------------------------------|DEFINES|---------------------------------- //

#define CLR_GREEN   "\033[38;5;46m"
#define CLR_RED     "\033[38;2;255;0;0m"
#define CLR_BLUE    "\033[38;2;0;157;255m"
#define CLR_YELLOW  "\033[38;2;255;255;0m"
#define CLR_RST     "\033[0m"

// ----------------------------------|STRUCTS|---------------------------------- //

typedef struct s_clientData {

    struct pollfd   poll;
    std::string     buffer;
    std::string     name;
    std::string     username;
    bool hasName;
    bool hasUsername;
    bool passwordAccepted;
    bool isRegistered;
    bool isOperator;
    
}              t_clientData;

// ----------------------------------|CLASSES|---------------------------------- //

class server {
    private:
        std::string _password;
        int _socket;
        int port;
    public:
        server();
        ~server();

        void setPassword(std::string input);
        bool checkPassword(std::string input);
};

server::server(){};
server::~server(){};

void server::setPassword(std::string input){
    this->_password = input;
}

bool server::checkPassword(std::string input){
    return (input == this->_password ? true : false);
}


// ----------------------------------|INITS|---------------------------------- //

// Initialize the server_socket, binds it and start listening
int init_server_socket(std::vector<t_clientData> &clientsData, std::vector<struct pollfd> &clients,int &serverSocket, const char *str_port);

// Get a new clientSocket to establish connection
int init_client_socket(int &serverSocket);

// ----------------------------------|NETWORK|---------------------------------- //

// Hamdle the different IRC features provided by the subject
bool handleEvents(std::vector<t_clientData> &clientsData, std::vector<struct pollfd> &clients);

// ----------------------------------|UTILS|---------------------------------- //

/*
Log in the proper stream using colors based on a Linux log level style
lvl options (s == SUCCESS | i == INFO | w == WARNING| e = ERROR)
*/
void logMsg(char lvl, std::string sender, std::string msg);

// Output the err message passed in parameter in stderr
void display_err(std::string err);

// Close server + all clients sockets fds
void closeAll(std::vector<struct pollfd> &clients);