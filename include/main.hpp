#pragma once

#include <iostream>
#include <string>

#define CLR_GREEN "\033[38;5;46m"
#define CLR_RED   "\033[38;2;255;0;0m"
#define CLR_BLUE  "\033[38;2;0;157;255m"
#define CLR_RST   "\033[0m"

// ERRORS
void display_err(std::string err);