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
#include <algorithm>
#include <cctype>
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

#define IRC_MAX_LINE    512             // Maximum size of an IRC line, \r\n included
#define MAX_SENDQ       (1024 * 1024)   // A client with more data waiting to be sent is disconnected

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

// Remove the socket from the members, operators and invites lists of every channel
void removeSocketFromChannels(server &server, int socket);

// Remove the socket from a single channel, deletes the channel if it has no members left
void removeSocketFromChannel(server &server, const std::string &channelName, int socket);

// Every socket sharing at least one channel with the provided socket, without duplicates or the socket itself
std::vector<int> getChannelPeers(server &server, int socket);

// Send QUIT to every user sharing a channel with the client, then remove him from all the channels
void quitChannels(server &server, ClientData &clientData, const std::string &reason);


// ----------------------------------|NETWORK|---------------------------------- //

// Nothing is sent directly: messages are queued in the client outBuffer, server::run() sends them when poll() reports POLLOUT
void queueMsg(ClientData &clientData, const std::string &msg);

void sendMsg(ClientData &clientData, std::string msg);

void sendPrivmsg(server &server, ClientData &clientData, int clientSocket, const std::string &target, const std::string &msg);

void broadcastMsg(server &server, ClientData &clientData, const std::vector<int> &channelMembersSockets, const std::string &target, const std::string &msg);