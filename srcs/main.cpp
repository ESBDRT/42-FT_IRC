#include "../include/main.hpp"

int main(int argc, const char **argv){

    // Check the number of arguments
    if (argc != 3)
        return (display_err("bad arguments: ./ircserv <port> <password>"), 1);

    // Create and initialize the server
    server ircServer;
    if (!ircServer.initialize(argv[1], std::string(argv[2])))
        return (1);

    // Infinite loop until run returns something
    while (true)
        if (!ircServer.run())
            break;
   
    return (0);
}
