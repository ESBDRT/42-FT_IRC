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
#include "serverClass.hpp"

// ----------------------------------|DEFINES|---------------------------------- //

#define CLR_GREEN   "\033[38;5;46m"
#define CLR_RED     "\033[38;2;255;0;0m"
#define CLR_BLUE    "\033[38;2;0;157;255m"
#define CLR_YELLOW  "\033[38;2;255;255;0m"
#define CLR_RST     "\033[0m"

// ----------------------------------|INITS|---------------------------------- //

// Get a new clientSocket to establish connection
int init_client_socket(int &serverSocket);

void handleCommands(server &server, t_clientData &clientData, std::string cmd);

// ----------------------------------|UTILS|---------------------------------- //

/*
Log in the proper stream using colors based on a Linux log level style
lvl options (s == SUCCESS | i == INFO | w == WARNING| e = ERROR)
*/
void logMsg(char lvl, std::string msg, std::string arg);

void logUserMsg(std::string user, std::string msg);

// Output the err message passed in parameter in stderr
void display_err(std::string err);


// ----------------------------------|NETWORK|---------------------------------- //

int sendMsg(t_clientData &clientData, int &clientSocket, std::string msg);