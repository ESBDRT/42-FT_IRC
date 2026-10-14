#include "../include/main.hpp"

int main(int argc, const char **argv)
{
    int server_sockt = -1;
    int client_sockt = -1;

    if (argc != 3)
        return (display_err("bad arguments: ./ircserv <port> <password>"), 1);

    if (init_server_socket(server_sockt, client_sockt, argv[1]))
        return (1);
   
    return (close(server_sockt), close(client_sockt), 0);
}
