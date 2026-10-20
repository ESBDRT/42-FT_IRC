#include "../../include/main.hpp"

// CHecker le nombre d'argument specifique a la tokens[0]

static int handlePASS (t_clientData &clientData, std::vector<std::string> tokens){

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

int handleCommands(t_clientData &clientData, std::string cmd){


    std::istringstream iss(cmd);
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token)
        tokens.push_back(token);

    if (tokens[0] == "PASS")
        return (handlePASS(clientData, tokens));        
    else if (tokens[0] == "NICK")
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

    // return (-1) = no tokens[0] found
    // return (0) == success
    // return (> 0) == failure
}