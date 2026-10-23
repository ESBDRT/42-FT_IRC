#include "../../include/main.hpp"

static void checkToggleRegistration(t_clientData &clientData){
    
    if (clientData.isRegistered)
        return;
    else if (clientData.hasUsername && clientData.hasName && clientData.passwordAccepted){
        clientData.isRegistered = true;
        logMsg('i', std::string (clientData.username + " is now registered."), "");
    }
    
}

static void handlePASS (server &server, t_clientData &clientData, std::vector<std::string> tokens){
    if (server.checkPassword(tokens[1])){
        clientData.passwordAccepted = true;
        logMsg('i', "Password accepted for client:", clientData.username);
        checkToggleRegistration(clientData);
    }
    else
        logMsg('e', "Invalid password provided by client:", clientData.username);
}

static void handleNICK (t_clientData &clientData, std::vector<std::string> tokens){
    
    if (clientData.username == tokens[1])
        return;

    std::string msg = "Username: " + clientData.username + " changed to:";
    clientData.username = tokens[1];
    clientData.hasUsername = true;
    
    logMsg('i', msg, clientData.username);

    checkToggleRegistration(clientData);
}

static void handleUSER (t_clientData &clientData, std::vector<std::string> tokens){
    
    std::string fullName;
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

    std::string msg = "Name: " + clientData.name + " changed to:";
    clientData.name = fullName;
    clientData.hasName = true;

    logMsg('i', msg, clientData.name);
    checkToggleRegistration(clientData);
}

// static void handleJOIN (t_clientData &clientData, std::vector<std::string> tokens){
//     if (!clientData.isRegistered){
//         logMsg('e', std::string (clientData.username + " must be registered to join channels"), "");
//         return;
//     }
// }

static void handlePRIVMSG (server &server, t_clientData &clientData, std::vector<std::string> tokens){

    if (!clientData.isRegistered){
        logMsg('e', std::string (clientData.username + " must be registered to send messages"), "");
        return;
    }

    if (!tokens[1].empty() && tokens[1][0] == '#')
            tokens[1].erase(0, 1);

    // #TODO: Make the function also search for channels with the same name
    int contactSocket = server.getContactSocket(tokens[1]);
    if (contactSocket == -1){
        logMsg('e', std::string (clientData.username + " tried to send a message but could not reach: " + tokens[1]), "");
        return;
    }

    std::string msg;
    for (size_t i = 2; i < tokens.size(); i++)
        msg += tokens[i] + ' ';

    if (sendMsg(clientData, contactSocket, msg) || sendMsg(clientData, clientData.poll.fd, msg)){
        logMsg('e', std::string (clientData.username + " failed to send message, something went wrong.."), "");
        return ;
    }
}   

// static void handleKICK (t_clientData &clientData, std::vector<std::string> tokens){

// }

// static void handleINVITE (t_clientData &clientData, std::vector<std::string> tokens){

// }

// static void handleTOPIC (t_clientData &clientData, std::vector<std::string> tokens){

// }

// static void handleMODE (t_clientData &clientData, std::vector<std::string> tokens){

// }

void handleCommands(server &server, t_clientData &clientData, std::string cmd){

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
        // else if (tokens[0] == "JOIN")
        //     handleJOIN(clientData, tokens);
        else if (tokens[0] == "PRIVMSG")
            handlePRIVMSG(server, clientData, tokens);
        // else if (tokens[0] == "KICK")
        //     handleKICK(clientData, tokens);
        // else if (tokens[0] == "INVITE")
        //     handleINVITE(clientData, tokens);
        // else if (tokens[0] == "TOPIC")
        //     handleTOPIC(clientData, tokens);
        // else if (tokens[0] == "MODE")
        //     handleMODE(clientData, tokens);
        
    }
}