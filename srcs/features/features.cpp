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

static void handleJOIN (server &server, t_clientData &clientData, std::vector<std::string> tokens){

    if (!clientData.isRegistered){
        logMsg('e', std::string (clientData.username + " must be registered to join channels"), "");
        return;
    }

    if (!tokens[1].empty() && tokens[1][0] == '#')
            tokens[1].erase(0, 1);

    std::vector<t_channel> &channelsList = server.getChannelsList();
    
    for (size_t i = 0; i < channelsList.size(); i++){                           // Search for an existing channel
        if (channelsList[i].name == tokens[1])
        {   
            std::vector<int> members = server.getChannelMembers(tokens[1]);
            for (size_t j = 0; j < members.size(); j++){
                if (clientData.poll.fd == members[j])
                {
                    logMsg('e', std::string(clientData.username + "is arleady in the channel"), tokens[1]);
                    sendMsg(clientData, clientData.poll.fd, std::string(clientData.username + " is arleady in the channel " + tokens[1]));
                    return ;
                }
            }
                
            channelsList[i].members.push_back(clientData.poll.fd);
            broadcastMsg(clientData, channelsList[i].members, std::string (clientData.username + " joined the " + tokens[1] + " channel"));
            logMsg('i', std::string (clientData.username + " joined the " + tokens[1] + " channel"), "");
            return ;
        }
    }

    t_channel newChannel;
    newChannel.currentUsers = 0;
    newChannel.inviteOnly = false;
    newChannel.isPasswordLocked = false;
    newChannel.maxUsers = 5;
    newChannel.members.push_back(clientData.poll.fd);
    newChannel.name = tokens[1];
    newChannel.operators.push_back(clientData.poll.fd);
    newChannel.password = "";
    newChannel.restrictedTopic = false;
    newChannel.topic = "";
    newChannel.userLimit = false;
    server.addNewChannel(newChannel);

    logMsg('i', std::string (clientData.username + " joined channel " + tokens[1]), "");
    sendMsg(clientData, clientData.poll.fd, std::string (" You joined the " + tokens[1] + " channel"));
}

static void handlePRIVMSG (server &server, t_clientData &clientData, std::vector<std::string> tokens){

    if (!clientData.isRegistered){
        logMsg('e', std::string (clientData.username + " must be registered to send messages"), "");
        return;
    }

    if (!tokens[1].empty() && tokens[1][0] == '#')
            tokens[1].erase(0, 1);

    std::string msg;
    for (size_t i = 2; i < tokens.size(); i++)
        msg += tokens[i] + ' ';

    int contactSocket = server.getContactSocket(tokens[1]);
    if (contactSocket == -1)
    {
        std::vector<int> channelMembers = server.getChannelMembers(tokens[1]);

        std::cout << "CHANNEL : " << tokens[1] << std::endl;

        std::cout << "NB MEMBERS : "
                  << channelMembers.size() << std::endl;

        for (size_t i = 0; i < channelMembers.size(); i++)
        {
            std::cout << "MEMBER FD : "
                      << channelMembers[i] << std::endl;
        }

        if (channelMembers.empty())
            logMsg('e', std::string (clientData.username + " could not find user/channel named: " + tokens[1]), "");
        else {
            msg.insert(0, std::string ("[" + tokens[1] + "]: "));
            broadcastMsg(clientData, channelMembers, msg);
            logMsg('i', std::string (clientData.username + " has sent a message in " + tokens[1]), "");
        }
        return;
    }

    if (sendMsg(clientData, contactSocket, msg) || sendMsg(clientData, clientData.poll.fd, msg)){
        logMsg('e', std::string (clientData.username + " failed to send message, something went wrong.."), "");
        return ;
    }

    logMsg('i', std::string (clientData.username + " has sent a message to " + tokens[1]), "");

}   

// static void handleKICK (t_clientData &clientData, std::vector<std::string> tokens){

// }

// static void handleINVITE (t_clientData &clientData, std::vector<std::string> tokens){

// }

// static void handleTOPIC (t_clientData &clientData, std::vector<std::string> tokens){

// }

// static void handleMODE (t_clientData &clientData, std::vector<std::string> tokens){

// }

// static void handlePING (t_clientData &clientData){

//     send(clientData.poll.fd, &clientData.buffer, )
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
        else if (tokens[0] == "JOIN")
            handleJOIN(server, clientData, tokens);
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
        else
            logMsg('e', "Unknown command: ", tokens[0]);
    }
}