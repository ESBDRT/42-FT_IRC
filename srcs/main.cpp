#include "../include/main.hpp"

volatile sig_atomic_t g_shutdown = 0;

// Only set the flag, the poll loop stops and the server destructor closes every socket
static void handleShutdownSignal(int signum){
    (void)signum;
    g_shutdown = 1;
}

// Shutdown properly on SIGINT (ctrl-C), SIGQUIT (ctrl-\) and SIGTERM, ignore SIGPIPE so a send() to a disconnected client doesn't kill the server
static bool setupSignals(){

    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);

    // No SA_RESTART, so poll() is interrupted with EINTR
    sa.sa_flags = 0;
    sa.sa_handler = handleShutdownSignal;
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGQUIT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1)
        return (display_err("sigaction: " + std::string(std::strerror(errno))), false);

    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) == -1)
        return (display_err("sigaction: " + std::string(std::strerror(errno))), false);

    return (true);
}

int main(int argc, const char **argv){

    // Check the number of arguments
    if (argc != 3)
        return (display_err("bad arguments: ./ircserv <port> <password>"), 1);

    if (!setupSignals())
        return (1);

    // Create and initialize the server
    server ircServer;
    if (!ircServer.initialize(argv[1], std::string(argv[2])))
        return (1);

    // Loop until run fails or a signal asks for a shutdown
    while (!g_shutdown)
        if (!ircServer.run())
            break;

    // Sockets are closed by the server destructor
    logMsg('i', "Shutting down the server", "");
    return (0);
}
