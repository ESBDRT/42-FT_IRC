#include "../include/main.hpp"

int main(int argc, const char **argv)
{
    int serverSocket;
    int clientSocket;

    if (argc != 3)
        return (display_err("bad arguments: ./ircserv <port> <password>"), 1);

    if (init_server_socket(serverSocket, argv[1]))
        return (1);

    while (true)
    {
        clientSocket = init_client_socket(serverSocket);

        if (clientSocket == -1)
            continue;

        logMsg('s', "NEW CLIENT CONNECTED");

        // For now, just close this client.
        close(clientSocket);
    }
   
    return (close(serverSocket), 0);
}
