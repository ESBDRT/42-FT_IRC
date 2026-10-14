#include "../../include/main.hpp"

int init_server_socket(int &serverSocket, const char *str_port)
{
    struct sockaddr_in addr;

    serverSocket = -1;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);                                     // Initialize the socket
    if (serverSocket == -1)
        return (display_err("socket: " + std::string(std::strerror(errno))), 1);

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

    logMsg('x', "Waiting for connections...");
    return (0);
}

int init_client_socket(int &serverSocket){

    int clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket == -1)
        return (display_err("accept: " + std::string(std::strerror(errno))), -1);
    else
        return (clientSocket);
}