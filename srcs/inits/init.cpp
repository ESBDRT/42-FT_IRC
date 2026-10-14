#include "../../include/main.hpp"

int init_server_socket(int &sockt, struct sockaddr_in &addr, const char *str_port)
{
    sockt = -1;
    sockt = socket(AF_INET, SOCK_STREAM, 0); // Initialize the socket
    if (sockt == -1)
        return (display_err("socket: " + std::string(std::strerror(errno))), 1);

    int port = check_port_conversion(str_port);
    if (port == -1)
        return (close(sockt), 1);

    addr.sin_family = AF_INET;                   // Which address family/type are you using (ipv4, v6, etc..)
    addr.sin_port = htons(port);                 // On which port is the socket listening
    addr.sin_addr.s_addr = INADDR_ANY;           // On which IP is the socket listening

    if (bind(sockt, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        display_err("bind: " + std::string(std::strerror(errno)));
        close(sockt);
        return (1);
    }

    return (0);
}