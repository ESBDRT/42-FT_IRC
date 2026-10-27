#include "../../include/main.hpp"
#include <fcntl.h>

// Applies non-blocking to the provided socket
static int set_nonblocking(int socket){

    int flags = fcntl(socket, F_GETFL, 0);
    if (flags == -1)
        return (display_err("fcntl(F_GETFL): " + std::string(std::strerror(errno))), 1);

    if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1)
        return (display_err("fcntl(F_SETFL): " + std::string(std::strerror(errno))), 1);

    return (0);
}

// Perform different checks to validate the provided port
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

bool server::initialize(const char *str_port, const std::string &password){

    // Sets the password in the object attributes
    setPassword(password);

    // Initialize the server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);                                 
    if (serverSocket == -1)
        return (display_err("socket: " + std::string(std::strerror(errno))), false);

    // Make the socket non blocking
    if (set_nonblocking(serverSocket)){
        close(serverSocket);
        return (false);
    }

    // Perform different checks to see if the port provided by the user is valid
    int port = check_port_conversion(str_port);
    if (port == -1)
        return (close(serverSocket), false);

    // Socket data structure
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;          // Which address family/type are you using (ipv4, v6, etc..)
    addr.sin_port = htons(port);        // On which port is the socket listening
    addr.sin_addr.s_addr = INADDR_ANY;  // On which IP is the socket listening

    // Bind the ServerSocket to the provided port
    if (bind(serverSocket, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        display_err("bind: " + std::string(std::strerror(errno)));
        close(serverSocket);
        return (false);
    }

    // Switch the serverSocket to listening state
    if (listen(serverSocket, SOMAXCONN) == -1){
        display_err("listen: " + std::string(std::strerror(errno)));
        close(serverSocket);
        return (false);
    }

    // Poll configuration for the listening socket
    struct pollfd servPoll;
    servPoll.fd = serverSocket;
    servPoll.events = POLLIN;
    servPoll.revents = 0;

    // Setting up client information
    ClientData servData;
    servData.buffer = "";
    servData.username = "SERVER";
    servData.name = "SERVER";
    servData.poll = servPoll;
    servData.hasName = true;
    servData.hasUsername = true;
    servData.isRegistered = true;
    servData.passwordAccepted = true;
    _clients.push_back(servData);

    logMsg('i', "Waiting for connections", "");
    return (true);
}

// Tries to create a socket and returns it or -1 on failure
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