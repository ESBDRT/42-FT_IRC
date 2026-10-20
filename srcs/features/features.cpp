#include "../../include/main.hpp"

static int handlePASS (server &server, t_clientData &clientData, std::vector<std::string> tokens){
    if (server.checkPassword(tokens[1])){
        clientData.passwordAccepted = true;
        logMsg('i', "Password accepted for client:", clientData.username);
        return (0);
    }
    else
        return (logMsg('e', "Invalid password for client:", clientData.username), 1);
}

static int handleNICK (){
    return (0);
}

static int handleUSER (){
    return (0);
}

static int handleJOIN (){
    return (0);
}

static int handlePRIVMSG (){
    return (0);
}

static int handleKICK (){
    return (0);
}

static int handleINVITE (){
    return (0);
}

static int handleTOPIC (){
    return (0);
}

static int handleMODE (){
    return (0);
}

int handleCommands(server &server, t_clientData &clientData, std::string cmd){

    std::istringstream iss(cmd);
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token)
        tokens.push_back(token);

    if (tokens[0] == "PASS" && tokens.size() == 2)
        return (handlePASS(server, clientData, tokens));        
    else if (tokens[0] == "NICK" && tokens.size() == 2 && clientData.passwordAccepted)
        return (handleNICK());
    else if (tokens[0] == "USER")
        return (handleUSER());
    else if (tokens[0] == "JOIN")
        return (handleJOIN());
    else if (tokens[0] == "PRIVMSG")
        return (handlePRIVMSG());
    else if (tokens[0] == "KICK")
        return (handleKICK());
    else if (tokens[0] == "INVITE")
        return (handleINVITE());
    else if (tokens[0] == "TOPIC")
        return (handleTOPIC());
    else if (tokens[0] == "MODE")
        return (handleMODE());
    else
        return (-1);

    // return (0) == success
    // return (1) == failure
}