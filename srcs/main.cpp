#include "../include/main.hpp"

int main(int argc, const char **argv)
{
    struct sockaddr_in addr;
    int sockt = -1;

    if (argc != 3)
        return (display_err("bad arguments: ./ircserv <port> <password>"), 1);

    if (init_server_socket(sockt, addr , argv[1]))
        return (close(sockt), 1);
   
    return (close(sockt), 1);
}
