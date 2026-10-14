#pragma once

#include <iostream>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <climits>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>


#define CLR_GREEN "\033[38;5;46m"
#define CLR_RED   "\033[38;2;255;0;0m"
#define CLR_BLUE  "\033[38;2;0;157;255m"
#define CLR_RST   "\033[0m"

// ----------------------------------|ERRORS|---------------------------------- //

// Output the err message passed in parameter in stderr
void display_err(std::string err);

// ----------------------------------|INITS|---------------------------------- //

// Initialize the socket, binds it and start listening
int init_server_socket(int &mySocket, struct sockaddr_in &addr, const char *str_port);

// ----------------------------------|CHECKS|---------------------------------- //

// Validates.
int check_port_conversion(const char *str_port);
