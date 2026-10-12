#pragma once

#include <iostream>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <climits>
#include <cstdlib>

#define CLR_GREEN "\033[38;5;46m"
#define CLR_RED   "\033[38;2;255;0;0m"
#define CLR_BLUE  "\033[38;2;0;157;255m"
#define CLR_RST   "\033[0m"

// ERRORS
void display_err(std::string err);

// ----------------------------------|CHECKS|---------------------------------- //

// Validates (or not) and sets the provided port if it is.
bool check_port_conversion(const char *str_port, int &port);
