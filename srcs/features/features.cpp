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

// Send a numeric reply (ex: ":localhost 475 nick #chan :Cannot join channel (+k)")
static void sendNumeric(ClientData &clientData, const std::string &code, const std::string &params){

    // Unregistered clients don't have a nickname yet, IRC uses * instead
    const std::string target = clientData.isRegistered ? clientData.username : "*";
    sendIrcFormattedMsg(clientData, ":localhost " + code + " " + target + " " + params);
}

// Build the source prefix of a message sent by a client (":nick!user@host")
static std::string userPrefix(const ClientData &clientData){
    return (":" + clientData.username + "!" + clientData.user + "@localhost");
}

// Send the same IRC line to every socket of the list, sender included
static void broadcastIrcLine(const std::vector<int> &sockets, std::string msg){

    // Add protocol message termination syntax
    msg += "\r\n";

    for (size_t i = 0; i < sockets.size(); ++i)
        if (send(sockets[i], msg.c_str(), msg.size(), 0) != static_cast<ssize_t>(msg.size()))
            logMsg('e', "Failed to broadcast IRC message:", msg.substr(0, msg.size() - 2));
}

// Check if the socket is in the channel members
static bool isMember(const Channel &channel, int clientSocket){

    for (size_t i = 0; i < channel.members.size(); ++i)
        if (channel.members[i] == clientSocket)
            return (true);
    return (false);
}

// Check if the socket has been invited to the channel
static bool isInvited(const Channel &channel, int clientSocket){

    for (size_t i = 0; i < channel.invited.size(); ++i)
        if (channel.invited[i] == clientSocket)
            return (true);
    return (false);
}

// Remove the socket from the channel invites, an invite can only be used once
static void removeInvite(Channel &channel, int clientSocket){

    for (size_t i = 0; i < channel.invited.size(); ++i){
        if (channel.invited[i] == clientSocket){
            channel.invited.erase(channel.invited.begin() + i);
            return ;
        }
    }
}

// Re-join the tokens from the provided index into a single trailing parameter, and remove the leading :
static std::string joinTrailing(const std::vector<std::string> &tokens, size_t start){

    std::string trailing;
    for (size_t i = start; i < tokens.size(); ++i){
        if (i > start)
            trailing += ' ';
        trailing += tokens[i];
    }

    if (!trailing.empty() && trailing[0] == ':')
        trailing.erase(0, 1);
    return (trailing);
}

// Announce the join to every member, then send the topic and the names list to the new member
static void sendJoinReplies(server &server, ClientData &clientData, Channel &channel){

    const std::string channelTarget = "#" + channel.name;

    broadcastIrcLine(channel.members, userPrefix(clientData) + " JOIN " + channelTarget);

    // 332 == RPL_TOPIC
    if (!channel.topic.empty())
        sendNumeric(clientData, "332", channelTarget + " :" + channel.topic);

    // Build the members list, operators are prefixed with @
    std::string names;
    for (size_t i = 0; i < channel.members.size(); ++i){
        int memberSocket = channel.members[i];
        if (!names.empty())
            names += ' ';
        if (server.isOperator(channel, memberSocket))
            names += '@';
        names += server.getNickname(memberSocket);
    }

    // 353 == RPL_NAMREPLY | 366 == RPL_ENDOFNAMES
    sendNumeric(clientData, "353", "= " + channelTarget + " :" + names);
    sendNumeric(clientData, "366", channelTarget + " :End of /NAMES list");
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

static void handleNICK (server &server, ClientData &clientData, std::vector<std::string> tokens){

    // Check if the new username == current username
    if (clientData.username == tokens[1])
        return;

    // Check that no other client is using this nickname (433 == ERR_NICKNAMEINUSE)
    if (server.getContactSocket(tokens[1]) != -1){
        logMsg('e', "Nickname already in use:", tokens[1]);
        sendNumeric(clientData, "433", tokens[1] + " :Nickname is already in use");
        return;
    }

    // Change the username
    std::string msg = "Username: " + clientData.username + " changed to:";
    clientData.username = tokens[1];
    clientData.hasUsername = true;
    
    logMsg('i', msg, clientData.username);
    sendMsg(clientData, clientData.poll.fd, std::string(" Your username is now " + clientData.username));

    // Check if the user has fully completed registration
    checkToggleRegistration(clientData);
}

// Sets the provided username and name, the nickname is only set by NICK
static void handleUSER (ClientData &clientData, std::vector<std::string> tokens){

    // USER can only be sent once, during registration (462 == ERR_ALREADYREGISTRED)
    if (clientData.isRegistered){
        sendNumeric(clientData, "462", ":You may not reregister");
        return;
    }

    // Build the full name, as it can have spaces
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

    // Sets username
    clientData.user = tokens[1];

    // Log name changes
    std::string msg = "Name: " + clientData.name + " changed to:";
    clientData.name = fullName;
    clientData.hasName = true;
    
    sendMsg(clientData, clientData.poll.fd, std::string(" Your name is now " + clientData.name));
    logMsg('i', msg, clientData.name);

    // Check if user completed registration
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

    // Check for client registration (451 == ERR_NOTREGISTERED)
    if (!clientData.isRegistered){
        logMsg('e', std::string (clientData.username + " must be registered to join channels"), "");
        sendNumeric(clientData, "451", ":You have not registered");
        return;
    }

    // Check that enough parameters are provided for that specific command (461 == ERR_NEEDMOREPARAMS)
    if (tokens.size() < 2){
        sendNumeric(clientData, "461", "JOIN :Not enough parameters");
        return ;
    }

    // Remove the # at index 0 from the channel name if there is any
    if (!tokens[1].empty() && tokens[1][0] == '#')
            tokens[1].erase(0, 1);

    const std::string channelTarget = "#" + tokens[1];

    // Try to Retrieve the channel info, returns a empty channel object if it doesn't exist
    Channel &targetChannel = server.getChannelInfo(tokens[1]);
    if (!targetChannel.name.empty())
    {
        // Check if the current user is arleady in the channel, IRC servers silently ignore it
        if (isMember(targetChannel, clientData.poll.fd)){
            logMsg('e', std::string(clientData.username + " is arleady in the channel"), tokens[1]);
            return ;
        }

        // Check if the channel is password locked, and if the provided password is corresponding (475 == ERR_BADCHANNELKEY)
        if (targetChannel.isPasswordLocked && (tokens.size() < 3 || targetChannel.password != tokens[2]))
        {
            logMsg('e', std::string(clientData.username + " provided a missing or invalid password for channel " + tokens[1]), "");
            sendNumeric(clientData, "475", channelTarget + " :Cannot join channel (+k)");
            return ;
        }

        // Check if the channel is invite only and if the user has been invited (473 == ERR_INVITEONLYCHAN)
        else if (targetChannel.inviteOnly && !isInvited(targetChannel, clientData.poll.fd))
        {
            logMsg('e', std::string(clientData.username + " tried to join the invite only channel " + tokens[1] + " without an invite"), "");
            sendNumeric(clientData, "473", channelTarget + " :Cannot join channel (+i)");
            return ;
        }

        // Every check passed, add the user to the channel members, consume the invite and log it
        else
        {
            targetChannel.members.push_back(clientData.poll.fd);
            removeInvite(targetChannel, clientData.poll.fd);
            sendJoinReplies(server, clientData, targetChannel);
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

        sendJoinReplies(server, clientData, newChannel);
        logMsg('i', std::string (clientData.username + " Created channel " + tokens[1]), "");        
        logMsg('i', std::string (clientData.username + " joined the " + tokens[1] + " channel"), "");        
    }

    return ;
}

// Handles privates messages sent to client and channels
static void handlePRIVMSG (server &server, ClientData &clientData, std::vector<std::string> tokens){

    // Check for client registration (451 == ERR_NOTREGISTERED)
    if (!clientData.isRegistered){
        logMsg('e', std::string (clientData.username + " must be registered to send messages"), "");
        sendNumeric(clientData, "451", ":You have not registered");
        return;
    }

    // Check that a target is provided (411 == ERR_NORECIPIENT)
    if (tokens.size() < 2){
        sendNumeric(clientData, "411", ":No recipient given (PRIVMSG)");
        return;
    }

    // Store the receiver name
    std::string target = tokens[1];

    // Build the entire message
    std::string msg;
    for (size_t i = 2; i < tokens.size(); i++){
        if (i > 2)
            msg += ' ';
        msg += tokens[i];
    }

    if (!msg.empty() && msg[0] == ':')
        msg.erase(0, 1);

    // Check that there is something to send (412 == ERR_NOTEXTTOSEND)
    if (msg.empty()){
        sendNumeric(clientData, "412", ":No text to send");
        return;
    }

    // If the target starts with a #, it's a channel
    if (target[0] == '#')
    {
        // Remove the # at index 0 from the channel name
        tokens[1].erase(0, 1);

        // Try to retrieve the channel info (403 == ERR_NOSUCHCHANNEL)
        Channel &targetChannel = server.getChannelInfo(tokens[1]);
        if (targetChannel.name.empty()){
            logMsg('e', std::string (clientData.username + " could not find channel named: " + tokens[1]), "");
            sendNumeric(clientData, "403", target + " :No such channel");
            return;
        }

        // Only members can talk in a channel (404 == ERR_CANNOTSENDTOCHAN)
        if (!isMember(targetChannel, clientData.poll.fd)){
            logMsg('e', std::string (clientData.username + " tried to send a message in " + tokens[1] + " but isn't a member"), "");
            sendNumeric(clientData, "404", target + " :Cannot send to channel");
            return;
        }

        // Send the message to every member in the channel except the sender
        broadcastMsg(clientData, targetChannel.members, target, msg);
        logMsg('i', std::string (clientData.username + " has sent a message in " + tokens[1]), "");
        return;
    }

    // Tries to find a USER corresponding to the target, returns the socket on success, and -1 on failure (401 == ERR_NOSUCHNICK)
    int contactSocket = server.getContactSocket(target);
    if (contactSocket == -1){
        logMsg('e', std::string (clientData.username + " could not find user named: " + target), "");
        sendNumeric(clientData, "401", target + " :No such nick/channel");
        return;
    }

    // Try to send the message to the user, the sender's client displays its own message so it isn't sent back
    if (sendPrivmsg(clientData, contactSocket, target, msg)){
        logMsg('e', std::string (clientData.username + " failed to send message, something went wrong.."), "");
        return ;
    }

    logMsg('i', std::string (clientData.username + " has sent a message to " + target), "");

}   

// Kick a user from a channel: KICK <#channel> <nick> [:reason]
static void handleKICK (server &server, ClientData &clientData, std::vector<std::string> tokens){

    // Check for client registration (451 == ERR_NOTREGISTERED)
    if (!clientData.isRegistered){
        sendNumeric(clientData, "451", ":You have not registered");
        return ;
    }

    // Check that enough parameters are provided for that specific command (461 == ERR_NEEDMOREPARAMS)
    if (tokens.size() < 3){
        sendNumeric(clientData, "461", "KICK :Not enough parameters");
        return ;
    }

    // Remove the # at index 0 from the channel name if there is any
    if (!tokens[1].empty() && tokens[1][0] == '#')
            tokens[1].erase(0, 1);

    const std::string channelTarget = "#" + tokens[1];
    const std::string &targetNick = tokens[2];

    // Try to retrieve the channel info (403 == ERR_NOSUCHCHANNEL)
    Channel &targetChannel = server.getChannelInfo(tokens[1]);
    if (targetChannel.name.empty()){
        sendNumeric(clientData, "403", channelTarget + " :No such channel");
        return ;
    }

    // The user must be in the channel (442 == ERR_NOTONCHANNEL)
    if (!isMember(targetChannel, clientData.poll.fd)){
        sendNumeric(clientData, "442", channelTarget + " :You're not on that channel");
        return ;
    }

    // Only operators can kick (482 == ERR_CHANOPRIVSNEEDED)
    if (!server.isOperator(targetChannel, clientData.poll.fd)){
        logMsg('e', std::string (clientData.username + " tried to kick " + targetNick + " from " + tokens[1] + " but isn't an operator"), "");
        sendNumeric(clientData, "482", channelTarget + " :You're not channel operator");
        return ;
    }

    // The kicked user must exist (401 == ERR_NOSUCHNICK) and be in the channel (441 == ERR_USERNOTINCHANNEL)
    int targetSocket = server.getContactSocket(targetNick);
    if (targetSocket == -1){
        sendNumeric(clientData, "401", targetNick + " :No such nick/channel");
        return ;
    }
    if (!isMember(targetChannel, targetSocket)){
        sendNumeric(clientData, "441", targetNick + " " + channelTarget + " :They aren't on that channel");
        return ;
    }

    // The reason is optional, IRC uses the kicker nickname by default
    std::string reason = joinTrailing(tokens, 3);
    if (reason.empty())
        reason = clientData.username;

    // Announce the kick to every member, kicked user included, before removing him
    broadcastIrcLine(targetChannel.members, userPrefix(clientData) + " KICK " + channelTarget + " " + targetNick + " :" + reason);
    logMsg('i', std::string (clientData.username + " kicked " + targetNick + " from " + tokens[1] + " (" + reason + ")"), "");

    // targetChannel must not be used after this call, the channel is deleted if it's now empty
    removeSocketFromChannel(server, tokens[1], targetSocket);
}

// Invite a user to a channel: INVITE <nick> <#channel>
static void handleINVITE (server &server, ClientData &clientData, std::vector<std::string> tokens){

    // Check for client registration (451 == ERR_NOTREGISTERED)
    if (!clientData.isRegistered){
        sendNumeric(clientData, "451", ":You have not registered");
        return ;
    }

    // Check that enough parameters are provided for that specific command (461 == ERR_NEEDMOREPARAMS)
    if (tokens.size() < 3){
        sendNumeric(clientData, "461", "INVITE :Not enough parameters");
        return ;
    }

    const std::string &targetNick = tokens[1];

    // Remove the # at index 0 from the channel name if there is any
    if (!tokens[2].empty() && tokens[2][0] == '#')
            tokens[2].erase(0, 1);

    const std::string channelTarget = "#" + tokens[2];

    // The invited user must exist (401 == ERR_NOSUCHNICK)
    int targetSocket = server.getContactSocket(targetNick);
    if (targetSocket == -1){
        sendNumeric(clientData, "401", targetNick + " :No such nick/channel");
        return ;
    }

    // Try to retrieve the channel info (403 == ERR_NOSUCHCHANNEL)
    Channel &targetChannel = server.getChannelInfo(tokens[2]);
    if (targetChannel.name.empty()){
        sendNumeric(clientData, "403", channelTarget + " :No such channel");
        return ;
    }

    // The user must be in the channel (442 == ERR_NOTONCHANNEL)
    if (!isMember(targetChannel, clientData.poll.fd)){
        sendNumeric(clientData, "442", channelTarget + " :You're not on that channel");
        return ;
    }

    // Only operators can invite in an invite only channel (482 == ERR_CHANOPRIVSNEEDED)
    if (targetChannel.inviteOnly && !server.isOperator(targetChannel, clientData.poll.fd)){
        logMsg('e', std::string (clientData.username + " tried to invite " + targetNick + " to " + tokens[2] + " but isn't an operator"), "");
        sendNumeric(clientData, "482", channelTarget + " :You're not channel operator");
        return ;
    }

    // The invited user must not be in the channel arleady (443 == ERR_USERONCHANNEL)
    if (isMember(targetChannel, targetSocket)){
        sendNumeric(clientData, "443", targetNick + " " + channelTarget + " :is already on channel");
        return ;
    }

    if (!isInvited(targetChannel, targetSocket))
        targetChannel.invited.push_back(targetSocket);

    // Confirm to the sender (341 == RPL_INVITING) and notify the invited user
    sendNumeric(clientData, "341", targetNick + " " + channelTarget);
    broadcastIrcLine(std::vector<int>(1, targetSocket), userPrefix(clientData) + " INVITE " + targetNick + " :" + channelTarget);
    logMsg('i', std::string (clientData.username + " invited " + targetNick + " to " + tokens[2]), "");
}

// View or change the topic of a channel: TOPIC <#channel> [:topic]
static void handleTOPIC (server &server, ClientData &clientData, std::vector<std::string> tokens){

    // Check for client registration (451 == ERR_NOTREGISTERED)
    if (!clientData.isRegistered){
        sendNumeric(clientData, "451", ":You have not registered");
        return ;
    }

    // Check that enough parameters are provided for that specific command (461 == ERR_NEEDMOREPARAMS)
    if (tokens.size() < 2){
        sendNumeric(clientData, "461", "TOPIC :Not enough parameters");
        return ;
    }

    // Remove the # at index 0 from the channel name if there is any
    if (!tokens[1].empty() && tokens[1][0] == '#')
            tokens[1].erase(0, 1);

    const std::string channelTarget = "#" + tokens[1];

    // Try to retrieve the channel info (403 == ERR_NOSUCHCHANNEL)
    Channel &targetChannel = server.getChannelInfo(tokens[1]);
    if (targetChannel.name.empty()){
        sendNumeric(clientData, "403", channelTarget + " :No such channel");
        return ;
    }

    // The user must be in the channel (442 == ERR_NOTONCHANNEL)
    if (!isMember(targetChannel, clientData.poll.fd)){
        sendNumeric(clientData, "442", channelTarget + " :You're not on that channel");
        return ;
    }

    // No topic provided, reply with the current one (331 == RPL_NOTOPIC | 332 == RPL_TOPIC)
    if (tokens.size() < 3){
        if (targetChannel.topic.empty())
            sendNumeric(clientData, "331", channelTarget + " :No topic is set");
        else
            sendNumeric(clientData, "332", channelTarget + " :" + targetChannel.topic);
        return ;
    }

    // Only operators can change the topic when the channel is +t (482 == ERR_CHANOPRIVSNEEDED)
    if (targetChannel.restrictedTopic && !server.isOperator(targetChannel, clientData.poll.fd)){
        logMsg('e', std::string (clientData.username + " tried to change the topic of " + tokens[1] + " but isn't an operator"), "");
        sendNumeric(clientData, "482", channelTarget + " :You're not channel operator");
        return ;
    }

    // Set the new topic ("TOPIC #channel :" clears it) and announce it to every member
    targetChannel.topic = joinTrailing(tokens, 2);
    broadcastIrcLine(targetChannel.members, userPrefix(clientData) + " TOPIC " + channelTarget + " :" + targetChannel.topic);
    logMsg('i', std::string (clientData.username + " changed the topic of " + tokens[1] + " to: " + targetChannel.topic), "");
}

static void handleMODE (server &server, ClientData &clientData, std::vector<std::string> tokens){

    // Check for client registration (451 == ERR_NOTREGISTERED)
    if (!clientData.isRegistered){
        sendNumeric(clientData, "451", ":You have not registered");
        return ;
    }

    // Check that a target is provided (461 == ERR_NEEDMOREPARAMS)
    if (tokens.size() < 2){
        sendNumeric(clientData, "461", "MODE :Not enough parameters");
        return ;
    }

    // User modes (MODE <nick> ...) are not handled, irssi sends one after registration so silently ignore it
    if (tokens[1][0] != '#')
        return ;

    // Store the channel target and remove the # at index 0 from the channel name
    const std::string channelTarget = tokens[1];
    tokens[1].erase(0, 1);

    // Try to retrieve the channel info (403 == ERR_NOSUCHCHANNEL)
    Channel &currentChannel = server.getChannelInfo(tokens[1]);
    if (currentChannel.name.empty()){
        logMsg('e', std::string (clientData.username + " tried MODE on channel " + tokens[1] + " but channel doesn't exist"), "");
        sendNumeric(clientData, "403", channelTarget + " :No such channel");
        return ;
    }

    // No mode provided, reply with the current channel modes (324 == RPL_CHANNELMODEIS)
    if (tokens.size() < 3){
        std::string modes = "+";
        if (currentChannel.inviteOnly)
            modes += 'i';
        if (currentChannel.restrictedTopic)
            modes += 't';
        if (currentChannel.isPasswordLocked)
            modes += 'k';
        sendNumeric(clientData, "324", channelTarget + " " + modes);
        return ;
    }

    // Check that the user is a channel operator (482 == ERR_CHANOPRIVSNEEDED)
    if (!server.isOperator(currentChannel, clientData.poll.fd)){
        logMsg('e', std::string (clientData.username + " tried " + tokens[2] + " on channel " + tokens[1] + " but isn't an operator"), "");
        sendNumeric(clientData, "482", channelTarget + " :You're not channel operator");
        return ;
    }

    // Every mode change is announced to all the channel members, sender included
    const std::string modeChange = userPrefix(clientData) + " MODE " + channelTarget + " ";

    // Toggle invite only, IRC servers silently ignore modes that are arleady set
    if (tokens[2] == "+i")
    {
        if (currentChannel.inviteOnly)
            return ;

        currentChannel.inviteOnly = true;
        logMsg('i', std::string ("Channel " + tokens[1] + " has been set to invite only by " + clientData.username), "");
        broadcastIrcLine(currentChannel.members, modeChange + "+i");
    }
    else if (tokens[2] == "-i")
    {
        if (!currentChannel.inviteOnly)
            return ;

        currentChannel.inviteOnly = false;
        logMsg('i', std::string ("Channel " + tokens[1] + " has been set to open by " + clientData.username), "");
        broadcastIrcLine(currentChannel.members, modeChange + "-i");
    }
    // else if (tokens[2] == "+t"){
    // }
    // else if (tokens[2] == "-t"){
    // }
    // Toggle password protection
    else if (tokens[2] == "+k")
    {
        // A password must be provided (461 == ERR_NEEDMOREPARAMS)
        if (tokens.size() < 4 || tokens[3].empty()){
            logMsg('e', std::string (clientData.username + " tried " + tokens[2] + " on channel " + tokens[1] + " but password cannot be empty"), "");
            sendNumeric(clientData, "461", "MODE :Not enough parameters");
            return ;
        }

        // The password must be removed before setting a new one (467 == ERR_KEYSET)
        if (currentChannel.isPasswordLocked){
            logMsg('e', std::string (clientData.username + " tried " + tokens[2] + " on channel " + tokens[1] + " but channel is arleady password locked"), "");
            sendNumeric(clientData, "467", channelTarget + " :Channel key already set");
            return ;
        }

        currentChannel.isPasswordLocked = true;
        currentChannel.password = tokens[3];
        logMsg('i', std::string ("Channel " + tokens[1] + " has been password locked by " + clientData.username), "");
        broadcastIrcLine(currentChannel.members, modeChange + "+k " + tokens[3]);
    }
    else if (tokens[2] == "-k")
    {
        if (!currentChannel.isPasswordLocked)
            return ;

        currentChannel.password = "";
        currentChannel.isPasswordLocked = false;
        logMsg('i', std::string ("Channel " + tokens[1] + " has been password unlocked by " + clientData.username), "");
        broadcastIrcLine(currentChannel.members, modeChange + "-k *");
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
    // Unknown mode (472 == ERR_UNKNOWNMODE)
    else
    {
        logMsg('e', std::string (clientData.username + " tried MODE " + tokens[2] + ": unknown parameter"), "");
        sendNumeric(clientData, "472", tokens[2] + " :is unknown mode char to me");
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
            handleNICK(server, clientData, tokens);
        else if (tokens[0] == "USER" && tokens.size() >= 5)
            handleUSER(clientData, tokens);
        else if (tokens[0] == "JOIN")
            handleJOIN(server, clientData, tokens);
        else if (tokens[0] == "PRIVMSG")
            handlePRIVMSG(server, clientData, tokens);
        else if (tokens[0] == "KICK")
            handleKICK(server, clientData, tokens);
        else if (tokens[0] == "INVITE")
            handleINVITE(server, clientData, tokens);
        else if (tokens[0] == "TOPIC")
            handleTOPIC(server, clientData, tokens);
        else if (tokens[0] == "MODE")
            handleMODE(server, clientData, tokens);
        else if (tokens[0] == "PING")
            handlePING(clientData, tokens);
        else
            logMsg('w', "Unknown command: ", tokens[0]);
    }
}