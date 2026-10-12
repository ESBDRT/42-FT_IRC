#include "../include/main.hpp"

int main(int argc, const char **argv)
{
    if (argc != 3)
        return (display_err(": bad arguments: ./ircserv <port> <password>"), 1);

    (void)argv;
    return 0;
}
