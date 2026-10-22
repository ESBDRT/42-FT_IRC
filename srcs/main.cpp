#include "../include/main.hpp"

int main(int argc, const char **argv){

    if (argc != 3)
        return (display_err("bad arguments: ./ircserv <port> <password>"), 1);

    server ircServer;
    if (!ircServer.initialize(argv[1], std::string(argv[2])))
        return (1);

    while (true)
        if (!ircServer.run())
            break;
   
    return (0);
}
