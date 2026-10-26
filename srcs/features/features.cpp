#include "../../include/main.hpp"

static void checkToggleRegistration(t_clientData &clientData){
    
    if (clientData.isRegistered)
        return;
    else if (clientData.hasUsername && clientData.hasName && clientData.passwordAccepted){
        clientData.isRegistered = true;
        logMsg('i', std::string (clientData.username + " is now registered."), "");
        sendMsg(clientData, clientData.poll.fd, " You are now registered");
    }
    
}

static void handlePASS (server &server, t_clientData &clientData, std::vector<std::string> tokens){
    if (server.checkPassword(tokens[1])){
        clientData.passwordAccepted = true;
        logMsg('i', "Password accepted for client:", clientData.username);
        sendMsg(clientData, clientData.poll.fd, " Password accepted");
        checkToggleRegistration(clientData);
    }
    else {
        logMsg('e', "Invalid password provided by client:", clientData.username);
        sendMsg(clientData, clientData.poll.fd, "Invalid password, try again");
    }
}

static void handleNICK (t_clientData &clientData, std::vector<std::string> tokens){
    
    if (clientData.username == tokens[1])
        return;

    std::string msg = "Username: " + clientData.username + " changed to:";
    clientData.username = tokens[1];
    clientData.hasUsername = true;
    
    logMsg('i', msg, clientData.username);
    sendMsg(clientData, clientData.poll.fd, std::string(" Your username is now " + clientData.username));

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
    
    sendMsg(clientData, clientData.poll.fd, std::string(" Your name is now " + clientData.username));
    logMsg('i', msg, clientData.name);
    checkToggleRegistration(clientData);
}

// TODO: MODE est cassew
static void handleJOIN (server &server, t_clientData &clientData, std::vector<std::string> tokens){

    if (!clientData.isRegistered){
        logMsg('e', std::string (clientData.username + " must be registered to join channels"), "");
        return;
    }

    if (tokens.size() < 2){
        sendMsg(clientData, clientData.poll.fd, std::string("Not enough parameters for JOIN"));
        return ;
    }

    t_channel &targetChannel = server.getChannelInfo(tokens[1]);
    if (!targetChannel.name.empty())
    {
        for (size_t j = 0; j < targetChannel.members.size(); j++){
            if (clientData.poll.fd == targetChannel.members[j])
            {
                logMsg('e', std::string(clientData.username + " is arleady in the channel"), tokens[1]);
                sendMsg(clientData, clientData.poll.fd, std::string(clientData.username + " is arleady in the channel " + tokens[1]));
                return ;
            }
        }
    
        if (targetChannel.isPasswordLocked && (tokens.size() < 3 || tokens[2].empty()))
        {
            logMsg('e', std::string(clientData.username + " no password provided for channel " + tokens[1]), "");
            sendMsg(clientData, clientData.poll.fd, std::string(" Channel " + tokens[1] + " is password protected"));
            return ;
        }   
        else if (targetChannel.isPasswordLocked && targetChannel.password != tokens[2])
        {
            logMsg('e', std::string(clientData.username + "invalid password provided for channel " + tokens[1]), "");
            sendMsg(clientData, clientData.poll.fd, std::string(" Access denied: invalid password for channel " + tokens[1]));
            return ;
        }
        else 
        {
            targetChannel.members.push_back(clientData.poll.fd);
            broadcastMsg(clientData, targetChannel.members, std::string (clientData.username + " joined the " + tokens[1] + " channel"));
            logMsg('i', std::string (clientData.username + " joined the " + tokens[1] + " channel"), "");
            return ;
        }
    }
    else 
    {
        t_channel newChannel;

        newChannel.currentUsers = 1;
        newChannel.inviteOnly = false;
        newChannel.isPasswordLocked = false;
        newChannel.maxUsers = 10;
        newChannel.name = tokens[1];
        newChannel.operators.push_back(clientData.poll.fd);
        newChannel.password = "";
        newChannel.restrictedTopic = false;
        newChannel.topic = "";
        newChannel.userLimit = false;
        newChannel.members.push_back(clientData.poll.fd);
        server.addNewChannel(newChannel);
        
        broadcastMsg(clientData, newChannel.members, std::string (clientData.username + " joined the " + tokens[1] + " channel"));
        logMsg('i', std::string (clientData.username + " Created channel " + tokens[1]), "");        
        logMsg('i', std::string (clientData.username + " joined the " + tokens[1] + " channel"), "");        
    }

    return ;
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

static void handleMODE (server &server, t_clientData &clientData, std::vector<std::string> tokens){

    if (tokens.size() < 3)
    {
        sendMsg(clientData, clientData.poll.fd, std::string("Not enough parameters for MODE"));
        return ;
    }

    t_channel &currentChannel = server.getChannelInfo(tokens[1]);
    if (!currentChannel.name.empty())
    {
        if (!server.isOperator(currentChannel, clientData.poll.fd)){
            logMsg('e', std::string (clientData.username + "tried" + tokens[2] + " on channel " + tokens[1] + " but isn't an operator"), "");               
            sendMsg(clientData, clientData.poll.fd, std::string("Permission denied, you must be an opeartor to use the MODE command"));
            return ;
        }
        else 
        {
            // Toggle invite only
            if (tokens[2] == "+i")
            {   
                if (!currentChannel.inviteOnly)
                {
                    currentChannel.inviteOnly = true;
                    logMsg('i', std::string ("Channel " + tokens[1] + " has been set to invite only by " + clientData.username), "");
                    broadcastMsg(clientData, currentChannel.members, std::string("This channel is now invite only"));                    
                }
                else 
                {
                    logMsg('e', std::string (clientData.username + "tried" + tokens[2] + " on channel " + tokens[1] + " but channel is arleady set to invite only"), "");
                    sendMsg(clientData, clientData.poll.fd, std::string("Channel is arleady set to invite only"));
                }
            }
            else if (tokens[2] == "-i")
            {
                if (currentChannel.inviteOnly)
                {
                    currentChannel.inviteOnly = false;
                    logMsg('i', std::string ("Channel " + tokens[1] + " has been set to open by " + clientData.username), "");
                    broadcastMsg(clientData, currentChannel.members, std::string("This channel is now open"));
                    return ;        
                }
                else 
                {
                    logMsg('e', std::string (clientData.username + "tried" + tokens[2] + " on channel " + tokens[1] + " but channel is arleady set to open"), "");
                    sendMsg(clientData, clientData.poll.fd, std::string("Channel is arleady set to invite open"));
                    return ;
                }
            }
            // else if (tokens[2] == "+t"){
            // }
            // else if (tokens[2] == "-t"){
            // }
            // Toggle password protection
            else if (tokens[2] == "+k")
            {
                if (!currentChannel.isPasswordLocked)
                {
                    if (tokens.size() < 4 || tokens[3].empty()){
                        logMsg('e', std::string (clientData.username + "tried" + tokens[2] + " on channel " + tokens[1] + " but password cannot be empty"), "");
                        sendMsg(clientData, clientData.poll.fd, std::string("Password cannot be empty"));
                        return ;
                    }
                    else
                    {
                        currentChannel.isPasswordLocked = true;
                        currentChannel.password = tokens[3];
                        logMsg('i', std::string ("Channel " + tokens[1] + " has been password locked by " + clientData.username), "");
                        broadcastMsg(clientData, currentChannel.members, std::string("This channel is now password locked"));
                        return ;                
                    }
                }
                else 
                {
                    logMsg('e', std::string (clientData.username + "tried" + tokens[2] + " on channel " + tokens[1] + " but channel is arleady password locked"), "");
                    sendMsg(clientData, clientData.poll.fd, std::string("Channel is arleady password locked"));
                }
            }
            else if (tokens[2] == "-k")
            {
                if (currentChannel.isPasswordLocked)
                {
                    currentChannel.password = "";
                    currentChannel.isPasswordLocked = false;
                    logMsg('i', std::string ("Channel " + tokens[1] + " has been password unlocked by " + clientData.username), "");
                    broadcastMsg(clientData, currentChannel.members, std::string("This channel is not password locked anymore"));
                }
                else 
                {
                    logMsg('e', std::string (clientData.username + "tried " + tokens[2] + " on channel " + tokens[1] + " but channel is arleady password unlocked"), "");
                    sendMsg(clientData, clientData.poll.fd, std::string("Channel is arleady password unlocked"));
                }
            }
            // else if (tokens[2] == "+o")
            // {

            // }
            // else if (tokens[2] == "-o")
            // {

            // }
            // else if (tokens[2] == "+l")
            // {

            // }
            // else if (tokens[2] == "-l")
            // {

            // }
            else 
            {
                logMsg('e', std::string (clientData.username + " tried MODE " + tokens[2] + ": unknown parameter"), "");
                sendMsg(clientData, clientData.poll.fd, std::string("unknown parameter: " + tokens[2]));
                return ;
            }
        }
    }
    else {
        logMsg('e', std::string (clientData.username + " tried" + tokens[2] + " on channel " + tokens[1] + " but channel doesn't exist"), "");
        sendMsg(clientData, clientData.poll.fd, std::string("Channel " + tokens[1] + " doesn't exist"));
        return ;
    }
}
    

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
        else if (tokens[0] == "MODE")
            handleMODE(server, clientData, tokens);
        else
            logMsg('w', "Unknown command: ", tokens[0]);
    }
}