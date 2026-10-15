#include "../../include/main.hpp"
#include <fcntl.h>

static int set_nonblocking(int socket){

    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1)
        return (display_err("fcntl(F_GETFL): " + std::string(std::strerror(errno))), 1);

    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1)
        return (display_err("fcntl(F_SETFL): " + std::string(std::strerror(errno))), 1);

    return (0);
}

static int check_port_conversion(const char *str_port){

    char *end;
    errno = 0;

    long port = strtol(str_port, &end, 10);

    if (end == str_port || *end != '\0')
        return (display_err("bad arguments: port must be only digits"), -1);
    
    if (errno == ERANGE)
        return (display_err("bad arguments: overflow/underflow"), -1);

    if (port < 1 || port > 65535)
        return (display_err("bad arguments: invalid TCP port range (must be 1-65535)"), -1);

    port = static_cast<int>(port);
    return (port);
}

int init_server_socket(std::vector<struct pollfd> &clients,int &serverSocket, const char *str_port){

    struct sockaddr_in addr;

    serverSocket = -1;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);                                     // Initialize the socket
    if (serverSocket == -1)
        return (display_err("socket: " + std::string(std::strerror(errno))), 1);

    if (set_nonblocking(serverSocket)){
        close(serverSocket);
        serverSocket = -1;
        return (1);
    }

    int port = check_port_conversion(str_port);
    if (port == -1)
        return (close(serverSocket), 1);

    addr.sin_family = AF_INET;                                                          // Which address family/type are you using (ipv4, v6, etc..)
    addr.sin_port = htons(port);                                                        // On which port is the socket listening
    addr.sin_addr.s_addr = INADDR_ANY;                                                  // On which IP is the socket listening

    if (bind(serverSocket, (struct sockaddr*)&addr, sizeof(addr)) == -1){               // Bind the ServerSocket to the provided port
        display_err("bind: " + std::string(std::strerror(errno)));
        close(serverSocket);
        return (1);
    }

    if (listen(serverSocket, SOMAXCONN) == -1){                                         // Make the serverSocket go into listening state
        display_err("listen: " + std::string(std::strerror(errno)));
        close(serverSocket);
        return (1);
    }

    struct pollfd serv;

    serv.fd = serverSocket;
    serv.events = POLLIN;
    serv.revents = 0;
    clients.push_back(serv);    

    logMsg('x', "[SERVER]", "Waiting for connections...");
    return (0);
}

int init_client_socket(int &serverSocket){

    int clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket == -1)
        return (display_err("accept: " + std::string(std::strerror(errno))), -1);

    if (set_nonblocking(clientSocket)){
        close(clientSocket);
        return (-1);
    }
    return (clientSocket);
}