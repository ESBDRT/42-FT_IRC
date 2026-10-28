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
#include <csignal>
#include "serverClass.hpp"

// Set by the signal handler when the server must shut down
extern volatile sig_atomic_t g_shutdown;

// ----------------------------------|DEFINES|---------------------------------- //

#define CLR_GREEN   "\033[38;5;46m"
#define CLR_RED     "\033[38;2;255;0;0m"
#define CLR_BLUE    "\033[38;2;0;157;255m"
#define CLR_YELLOW  "\033[38;2;255;255;0m"
#define CLR_RST     "\033[0m"

// ----------------------------------|INITS|---------------------------------- //

// Get a new clientSocket to establish connection
int init_client_socket(int &serverSocket);

void handleCommands(server &server, ClientData &clientData, std::string cmd);

// ----------------------------------|UTILS|---------------------------------- //

/*
Log in the proper stream using colors based on a Linux log level style
lvl options (s == SUCCESS | i == INFO | w == WARNING| e = ERROR)
*/
void logMsg(char lvl, std::string msg, std::string arg);

// Output the err message passed in parameter in stderr
void display_err(std::string err);

// Remove the socket from the members and operators lists of every channel
void removeSocketFromChannels(server &server, int socket);


// ----------------------------------|NETWORK|---------------------------------- //

int sendMsg(ClientData &clientData, int &clientSocket, std::string msg);

int sendPrivmsg(ClientData &clientData, int &clientSocket, const std::string &target, const std::string &msg);

void broadcastMsg(ClientData &clientData, std::vector<int> channelMembersSockets, const std::string &target, const std::string &msg);