#include "../../include/main.hpp"

static bool sendIrcFormattedMsg(ClientData &clientData, std::string msg){

    // Add protocol message termination syntax
    msg += "\r\n";

    // Send the message
    ssize_t bytesSent = send(clientData.poll.fd, msg.c_str(), msg.size(), 0);
    if (bytesSent != static_cast<ssize_t>(msg.size())){
        logMsg('e', "Failed to send IRC response to:", clientData.username);
        return (false);
    }
    return (true);
}

// Broadcast the message to every user in a channel + log sender
static void broadcastChannelNotice(ClientData &clientData, const std::vector<int> &channelMembersSockets, const std::string &target, const std::string &msg){

    broadcastMsg(clientData, channelMembersSockets, target, msg);
    if (sendPrivmsg(clientData, clientData.poll.fd, target, msg))
        logMsg('e', std::string(clientData.username + " failed to receive channel message"), target);
}

static void checkToggleRegistration(ClientData &clientData){
    
    // Return if user is arleady registered
    if (clientData.isRegistered)
        return;

    // Check if the user has a complete profile
    else if (clientData.hasUsername && clientData.hasName && clientData.passwordAccepted){
        clientData.isRegistered = true;
        logMsg('i', std::string (clientData.username + " is now registered."), "");
        
        // Send IRC registration validation
        sendIrcFormattedMsg(clientData, std::string(":localhost 001 " + clientData.username + " :Welcome to the IRC Network " + clientData.username));
    }
}

static void handlePASS (server &server, ClientData &clientData, std::vector<std::string> tokens){

    // Check if the provided password is valid
    if (server.checkPassword(tokens[1])){
        clientData.passwordAccepted = true;
        logMsg('i', "Password accepted for client:", clientData.username);
        sendMsg(clientData, clientData.poll.fd, " Password accepted");
        checkToggleRegistration(clientData);
    }
    else {
        // Log errors
        logMsg('e', "Invalid password provided by client:", clientData.username);
        sendMsg(clientData, clientData.poll.fd, " Invalid password, try again");
    }
}

static void handleNICK (ClientData &clientData, std::vector<std::string> tokens){
    
    // Check if the new username == current username
    if (clientData.username == tokens[1])
        return;

    // Change the username
    std::string msg = "Username: " + clientData.username + " changed to:";
    clientData.username = tokens[1];
    clientData.hasUsername = true;
    
    logMsg('i', msg, clientData.username);
    sendMsg(clientData, clientData.poll.fd, std::string(" Your username is now " + clientData.username));

    // Check if the user has fully completed registration
    checkToggleRegistration(clientData);
}

// TODO: Needs rework (command is named userhost according to logs)
static void handleUSER (ClientData &clientData, std::vector<std::string> tokens){
    
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
    
    sendMsg(clientData, clientData.poll.fd, std::string(" Your name is now " + clientData.name));
    logMsg('i', msg, clientData.name);
    checkToggleRegistration(clientData);
}

static void handleCAP(ClientData &clientData, const std::vector<std::string> &tokens){

    // Capabilities are optional protocol features such as messages tags, account related notifications, etc.. 

    // Check if there is enough arguments for that specific command
    if (tokens.size() < 2)
        return;

    // If the client is registered, use his name instead
    std::string target = clientData.isRegistered ? clientData.username : "*";

    // LS asks for the server list of capabilities
    if (tokens[1] == "LS")
        sendIrcFormattedMsg(clientData, ":localhost CAP " + target + " LS :");

    // Lists asks for a list of the currently active capabilities
    else if (tokens[1] == "LIST")
        sendIrcFormattedMsg(clientData, ":localhost CAP " + target + " LIST :");
    
    // REQ asks for a capability to be turned on/off
    else if (tokens[1] == "REQ")
    {
        std::string requestedCaps;
        
        // Build a string with all the requested capabilities + NAK (NAK == rejection)
        for (size_t i = 2; i < tokens.size(); ++i){
            if (!requestedCaps.empty())
                requestedCaps += ' ';
            requestedCaps += tokens[i];
        }

        // Remove the leading : used by irc to mark a trailing parameter
        if (!requestedCaps.empty() && requestedCaps[0] == ':')
            requestedCaps.erase(0, 1);
        
        // Send message
        sendIrcFormattedMsg(clientData, ":localhost CAP " + target + " NAK :" + requestedCaps);
    }
}

// Handle JOIN command for channels
static void handleJOIN (server &server, ClientData &clientData, std::vector<std::string> tokens){

    // Check for client registration 
    if (!clientData.isRegistered){
        logMsg('e', std::string (clientData.username + " must be registered to join channels"), "");
        return;
    }

    // Check that enough parameters are provided for that specific command
    if (tokens.size() < 2){
        sendMsg(clientData, clientData.poll.fd, std::string("Not enough parameters for JOIN"));
        return ;
    }

    // Remove the # at index 0 from the channel name if there is any
    if (!tokens[1].empty() && tokens[1][0] == '#')
            tokens[1].erase(0, 1);

    // Try to Retrieve the channel info, returns a empty channel object if it doesn't exist 
    Channel &targetChannel = server.getChannelInfo(tokens[1]);
    if (!targetChannel.name.empty())
    {
        // Iterate trough the channel members and check if the current user is arleady in the channel
        for (size_t j = 0; j < targetChannel.members.size(); j++){
            if (clientData.poll.fd == targetChannel.members[j])
            {
                logMsg('e', std::string(clientData.username + " is arleady in the channel"), tokens[1]);
                sendMsg(clientData, clientData.poll.fd, std::string(" You are arleady in the " + tokens[1] + " channel"));
                return ;
            }
        }
    
        // Check if the channel is password locked, and if there is enough arguments and that it's not empty
        if (targetChannel.isPasswordLocked && (tokens.size() < 3 || tokens[2].empty()))
        {
            logMsg('e', std::string(clientData.username + " attempted to join the password locked channel" + tokens[1] + "without providing a password"), "");
            sendMsg(clientData, clientData.poll.fd, std::string(" Channel " + tokens[1] + " is password protected"));
            return ;
        }   

        // Check if the provided password is corresponding to the channel password
        else if (targetChannel.isPasswordLocked && targetChannel.password != tokens[2])
        {
            logMsg('e', std::string(clientData.username + " provided an invalid password for channel " + tokens[1]), "");
            sendMsg(clientData, clientData.poll.fd, std::string(" Access denied: invalid password for channel " + tokens[1]));
            return ;
        }

        // Password is correct, add the user to the channel members and log it
        else 
        {
            targetChannel.members.push_back(clientData.poll.fd);
            const std::string channelTarget = "#" + targetChannel.name;
            const std::string notice = clientData.username + " joined the " + targetChannel.name + " channel";
            broadcastChannelNotice(clientData, targetChannel.members, channelTarget, notice);
            logMsg('i', std::string (clientData.username + " joined the " + tokens[1] + " channel"), "");
            return ;
        }
    }
    
    // If the channel doesn't exist, create it
    else 
    {
        // Create a new channel object with default settings and add it to the channels list
        Channel newChannel;

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
        
        const std::string channelTarget = "#" + newChannel.name;
        const std::string notice = clientData.username + " joined the " + newChannel.name + " channel";
        broadcastChannelNotice(clientData, newChannel.members, channelTarget, notice);
        logMsg('i', std::string (clientData.username + " Created channel " + tokens[1]), "");        
        logMsg('i', std::string (clientData.username + " joined the " + tokens[1] + " channel"), "");        
    }

    return ;
}

// Handles privates messages sent to client and channels
static void handlePRIVMSG (server &server, ClientData &clientData, std::vector<std::string> tokens){

    // Check for client registration
    if (!clientData.isRegistered){
        logMsg('e', std::string (clientData.username + " must be registered to send messages"), "");
        return;
    }

    // Check that enough parameters are provided for that specific command
    if (tokens.size() < 3){
        sendMsg(clientData, clientData.poll.fd, " Not enough parameters for PRIVMSG");
        return;
    }

    // Store the receiver name and remove the # at index 0 if there is any
    std::string target = tokens[1];
    if (!tokens[1].empty() && tokens[1][0] == '#')
            tokens[1].erase(0, 1);

    // Build the entire message 
    std::string msg;
    for (size_t i = 2; i < tokens.size(); i++){
        if (i > 2)
            msg += ' ';
        msg += tokens[i];
    }

    if (!msg.empty() && msg[0] == ':')
        msg.erase(0, 1);

    // Tries to find a USER corresponding to the target, returns the socket on success, and -1 on failure
    int contactSocket = server.getContactSocket(tokens[1]);

    // If no USER is found, search for a channel
    if (contactSocket == -1)
    {
        // Try to retrieve the list of users for the target channel
        std::vector<int> channelMembers = server.getChannelMembers(tokens[1]);
       
        // If the channel has no member, it means that it's non existing
        // #TODO: A channel having a empty member lists doesn't necessarly mean it doesn't exist, unless we delete the channel when everyone leaves
        if (channelMembers.empty())
            logMsg('e', std::string (clientData.username + " could not find user/channel named: " + tokens[1]), "");

        // Send the message to every member in the channel
        else {
            broadcastMsg(clientData, channelMembers, target, msg);
            logMsg('i', std::string (clientData.username + " has sent a message in " + tokens[1]), "");
        }
        return;
    }

    // Try to send the message to the user
    if (sendPrivmsg(clientData, contactSocket, target, msg) || sendPrivmsg(clientData, clientData.poll.fd, target, msg)){
        logMsg('e', std::string (clientData.username + " failed to send message, something went wrong.."), "");
        return ;
    }

    logMsg('i', std::string (clientData.username + " has sent a message to " + tokens[1]), "");

}   

// static void handleKICK (ClientData &clientData, std::vector<std::string> tokens){

// }

// static void handleINVITE (ClientData &clientData, std::vector<std::string> tokens){

// }

// static void handleTOPIC (ClientData &clientData, std::vector<std::string> tokens){

// }

static void handleMODE (server &server, ClientData &clientData, std::vector<std::string> tokens){

    if (tokens.size() < 3)
    {
        sendMsg(clientData, clientData.poll.fd, std::string("Not enough parameters for MODE"));
        return ;
    }

    Channel &currentChannel = server.getChannelInfo(tokens[1]);
    if (!currentChannel.name.empty())
    {
        if (!server.isOperator(currentChannel, clientData.poll.fd)){
            logMsg('e', std::string (clientData.username + "tried" + tokens[2] + " on channel " + tokens[1] + " but isn't an operator"), "");               
            sendMsg(clientData, clientData.poll.fd, std::string(" Permission denied, you must be an opeartor to use the MODE command"));
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
                    broadcastChannelNotice(clientData, currentChannel.members, "#" + currentChannel.name, "This channel is now invite only");
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
                    broadcastChannelNotice(clientData, currentChannel.members, "#" + currentChannel.name, "This channel is now open");
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
                        broadcastChannelNotice(clientData, currentChannel.members, "#" + currentChannel.name, "This channel is now password locked");
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
                    broadcastChannelNotice(clientData, currentChannel.members, "#" + currentChannel.name, "This channel is not password locked anymore");
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

static void handlePING(ClientData &clientData, const std::vector<std::string> &tokens){

    // Origin == the msg that needs to be sent back with PONG
    
    // Check if a origin is provided (409 == ERR_NOORIGIN) 
    if (tokens.size() < 2){
        const std::string target = clientData.isRegistered ? clientData.username : "*";
        sendIrcFormattedMsg(clientData, ":localhost 409 " + target + " :No origin specified");
        return ;
    }

    // Build the response
    std::string origin;
    for (size_t i = 1; i < tokens.size(); ++i){
        if (i > 1)
            origin += ' ';
        origin += tokens[i];
    }

    // Remove the leading : used by irc to mark a trailing parameter
    if (!origin.empty() && origin[0] == ':')
        origin.erase(0, 1);

    // Send response
    sendIrcFormattedMsg(clientData, ":localhost PONG localhost :" + origin);
}

// Parse and handle commands
void handleCommands(server &server, ClientData &clientData, std::string cmd){

    std::istringstream iss(cmd);
    std::string token;
    std::vector<std::string> tokens;

    std::cout << "FULL COMMAND RECEIVED = " << cmd << std::endl;

    // Split the cmd parameter into multiple strings using space separator
    while (iss >> token)
        tokens.push_back(token);

    if (tokens.size() > 0)
    {
        if (tokens[0] == "CAP")
            handleCAP(clientData, tokens);
        else if (tokens[0] == "PASS" && tokens.size() == 2)
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
        else if (tokens[0] == "PING")
            handlePING(clientData, tokens);
        else
            logMsg('w', "Unknown command: ", tokens[0]);
    }
}