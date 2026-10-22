#include "../../include/main.hpp"

static void handlePASS (server &server, t_clientData &clientData, std::vector<std::string> tokens){
    if (server.checkPassword(tokens[1])){
        clientData.passwordAccepted = true;
        logMsg('i', "Password accepted for client:", clientData.username);
    }
    else
        logMsg('e', "Invalid password provided by client:", clientData.username);
}

static void handleNICK (t_clientData &clientData, std::vector<std::string> tokens){
    std::string msg = "Name: ";
    msg += clientData.name;
    msg += " changed to:";
    clientData.name = tokens[1];
    clientData.hasName = true;
    logMsg('i', msg, clientData.name);
}

static void handleUSER (t_clientData &clientData, std::vector<std::string> tokens){
    std::string fullName;
    std::string msg;
    
    size_t i = 4;

    while (i < tokens.size())
    {
        if (!tokens[i].empty() && tokens[i][0] == ':')
            tokens[i].erase(0, 1);

        fullName += tokens[i++];

        if (i < tokens.size())
            fullName += ' ';
    }

    handleNICK(clientData, tokens);
    msg += "Username: ";
    msg += clientData.username;
    msg += " changed to:";
    clientData.username = fullName;

    logMsg('i', msg, clientData.username);
}

static void handleJOIN (){
    
}

static void handlePRIVMSG (){

}   

static void handleKICK (){

}

static void handleINVITE (){

}

static void handleTOPIC (){

}

static void handleMODE (){

}

bool handleCommands(server &server, t_clientData &clientData, std::string cmd){

    std::istringstream iss(cmd);
    std::string token;
    std::vector<std::string> tokens;

    while (iss >> token)
        tokens.push_back(token);

    if (tokens.size() > 0)
    {
        if (tokens[0] == "PASS" && tokens.size() == 2)
            handlePASS(server, clientData, tokens);        
        else if (tokens[0] == "NICK" && tokens.size() == 2)
            handleNICK(clientData, tokens);
        else if (tokens[0] == "USER" && tokens.size() >= 5)
            handleUSER(clientData, tokens);
        else if (tokens[0] == "JOIN")
            handleJOIN();
        else if (tokens[0] == "PRIVMSG")
            handlePRIVMSG();
        else if (tokens[0] == "KICK")
            handleKICK();
        else if (tokens[0] == "INVITE")
            handleINVITE();
        else if (tokens[0] == "TOPIC")
            handleTOPIC();
        else if (tokens[0] == "MODE")
            handleMODE();
        
        return (true);
    }
    return (false);
}