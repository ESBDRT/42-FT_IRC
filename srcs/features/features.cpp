#include "../../include/main.hpp"

// Queue an IRC line for the client, the \r\n is added by queueMsg
static void sendIrcFormattedMsg(ClientData &clientData, const std::string &msg){
    queueMsg(clientData, msg);
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

// Queue the same IRC line for every socket of the list, sender included
static void broadcastIrcLine(server &server, const std::vector<int> &sockets, const std::string &msg){

    for (size_t i = 0; i < sockets.size(); ++i)
        server.queueMsg(sockets[i], msg);
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

// Remove the socket from the channel operators
static void removeOperator(Channel &channel, int clientSocket){

    for (size_t i = 0; i < channel.operators.size(); ++i){
        if (channel.operators[i] == clientSocket){
            channel.operators.erase(channel.operators.begin() + i);
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

    broadcastIrcLine(server, channel.members, userPrefix(clientData) + " JOIN " + channelTarget);

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

    // Check that a password is provided (461 == ERR_NEEDMOREPARAMS)
    if (tokens.size() < 2){
        sendNumeric(clientData, "461", "PASS :Not enough parameters");
        return;
    }

    // Check if the provided password is valid
    if (server.checkPassword(tokens[1])){
        clientData.passwordAccepted = true;
        logMsg('i', "Password accepted for client:", clientData.username);
        sendMsg(clientData, " Password accepted");
        checkToggleRegistration(clientData);
    }
    else {
        // Log errors
        logMsg('e', "Invalid password provided by client:", clientData.username);
        sendMsg(clientData, " Invalid password, try again");
    }
}

static void handleNICK (server &server, ClientData &clientData, std::vector<std::string> tokens){

    // Check that a nickname is provided (431 == ERR_NONICKNAMEGIVEN)
    if (tokens.size() < 2){
        sendNumeric(clientData, "431", ":No nickname given");
        return;
    }

    // Check if the new username == current username
    if (clientData.username == tokens[1])
        return;

    // Check that no other client is using this nickname (433 == ERR_NICKNAMEINUSE)
    if (server.getContactSocket(tokens[1]) != -1){
        logMsg('e', "Nickname already in use:", tokens[1]);
        sendNumeric(clientData, "433", tokens[1] + " :Nickname is already in use");
        return;
    }

    // Once registered, the change is announced to the user and to everyone sharing a channel with him, using the old nickname as prefix
    if (clientData.isRegistered){
        const std::string nickChange = userPrefix(clientData) + " NICK :" + tokens[1];
        sendIrcFormattedMsg(clientData, nickChange);

        std::vector<int> peers = getChannelPeers(server, clientData.poll.fd);
        for (size_t i = 0; i < peers.size(); i++)
            server.queueMsg(peers[i], nickChange);
    }

    // Change the username
    std::string msg = "Username: " + clientData.username + " changed to:";
    clientData.username = tokens[1];
    clientData.hasUsername = true;
    
    logMsg('i', msg, clientData.username);
    sendMsg(clientData, std::string(" Your username is now " + clientData.username));

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

    // USER <username> <mode> <unused> :<realname> (461 == ERR_NEEDMOREPARAMS)
    if (tokens.size() < 5){
        sendNumeric(clientData, "461", "USER :Not enough parameters");
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
    
    sendMsg(clientData, std::string(" Your name is now " + clientData.name));
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

// Channel lists (ex: "JOIN #a,#b") are not supported, returns true if the parameter is a list and has been refused (476 == ERR_BADCHANMASK)
// The refusal is sent for each channel of the list, irssi waits for an answer per channel and would never retry the command otherwise
static bool refuseChannelList(ClientData &clientData, const std::string &channels, const std::string &command){

    if (channels.find(',') == std::string::npos)
        return (false);

    logMsg('e', std::string (clientData.username + " tried to " + command + " several channels at once:"), channels);

    std::istringstream channelList(channels);
    std::string channelName;
    while (std::getline(channelList, channelName, ','))
        if (!channelName.empty())
            sendNumeric(clientData, "476", channelName + " :Bad Channel Mask, only one channel at a time");
    return (true);
}

// Join a single channel, or create it if it doesn't exist
static void joinChannel(server &server, ClientData &clientData, std::string channelName, const std::string &key){

    // Remove the # at index 0 from the channel name if there is any (403 == ERR_NOSUCHCHANNEL)
    if (!channelName.empty() && channelName[0] == '#')
        channelName.erase(0, 1);
    if (channelName.empty()){
        sendNumeric(clientData, "403", "# :No such channel");
        return ;
    }

    const std::string channelTarget = "#" + channelName;

    // Try to Retrieve the channel info, returns a empty channel object if it doesn't exist
    Channel &targetChannel = server.getChannelInfo(channelName);
    if (!targetChannel.name.empty())
    {
        // Check if the current user is arleady in the channel, IRC servers silently ignore it
        if (isMember(targetChannel, clientData.poll.fd)){
            logMsg('e', std::string(clientData.username + " is arleady in the channel"), channelName);
            return ;
        }

        // Check if the channel is password locked, and if the provided password is corresponding (475 == ERR_BADCHANNELKEY)
        if (targetChannel.isPasswordLocked && targetChannel.password != key)
        {
            logMsg('e', std::string(clientData.username + " provided a missing or invalid password for channel " + channelName), "");
            sendNumeric(clientData, "475", channelTarget + " :Cannot join channel (+k)");
            return ;
        }

        // Check if the channel is invite only and if the user has been invited (473 == ERR_INVITEONLYCHAN)
        else if (targetChannel.inviteOnly && !isInvited(targetChannel, clientData.poll.fd))
        {
            logMsg('e', std::string(clientData.username + " tried to join the invite only channel " + channelName + " without an invite"), "");
            sendNumeric(clientData, "473", channelTarget + " :Cannot join channel (+i)");
            return ;
        }

        // Check if the channel is full (471 == ERR_CHANNELISFULL)
        else if (targetChannel.userLimit && targetChannel.members.size() >= targetChannel.maxUsers)
        {
            logMsg('e', std::string(clientData.username + " tried to join the full channel " + channelName), "");
            sendNumeric(clientData, "471", channelTarget + " :Cannot join channel (+l)");
            return ;
        }

        // Every check passed, add the user to the channel members, consume the invite and log it
        else
        {
            targetChannel.members.push_back(clientData.poll.fd);
            removeInvite(targetChannel, clientData.poll.fd);
            sendJoinReplies(server, clientData, targetChannel);
            logMsg('i', std::string (clientData.username + " joined the " + channelName + " channel"), "");
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
        newChannel.name = channelName;
        newChannel.operators.push_back(clientData.poll.fd);
        newChannel.password = "";
        newChannel.restrictedTopic = false;
        newChannel.topic = "";
        newChannel.userLimit = false;
        newChannel.members.push_back(clientData.poll.fd);
        server.addNewChannel(newChannel);

        sendJoinReplies(server, clientData, newChannel);
        logMsg('i', std::string (clientData.username + " Created channel " + channelName), "");        
        logMsg('i', std::string (clientData.username + " joined the " + channelName + " channel"), "");        
    }

    return ;
}

// Handle JOIN command for channels: JOIN <#channel> [<key>], only one channel at a time
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

    // Refuse channel lists instead of creating a channel named "a,#b"
    if (refuseChannelList(clientData, tokens[1], "join"))
        return ;

    joinChannel(server, clientData, tokens[1], tokens.size() > 2 ? tokens[2] : "");
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
        broadcastMsg(server, clientData, targetChannel.members, target, msg);
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

    // Send the message to the user, the sender's client displays its own message so it isn't sent back
    sendPrivmsg(server, clientData, contactSocket, target, msg);

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
    broadcastIrcLine(server, targetChannel.members, userPrefix(clientData) + " KICK " + channelTarget + " " + targetNick + " :" + reason);
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
    server.queueMsg(targetSocket, userPrefix(clientData) + " INVITE " + targetNick + " :" + channelTarget);
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
    broadcastIrcLine(server, targetChannel.members, userPrefix(clientData) + " TOPIC " + channelTarget + " :" + targetChannel.topic);
    logMsg('i', std::string (clientData.username + " changed the topic of " + tokens[1] + " to: " + targetChannel.topic), "");
}

// Leave a channel: PART <#channel> [:reason], only one channel at a time
static void handlePART (server &server, ClientData &clientData, std::vector<std::string> tokens){

    // Check for client registration (451 == ERR_NOTREGISTERED)
    if (!clientData.isRegistered){
        sendNumeric(clientData, "451", ":You have not registered");
        return ;
    }

    // Check that enough parameters are provided for that specific command (461 == ERR_NEEDMOREPARAMS)
    if (tokens.size() < 2){
        sendNumeric(clientData, "461", "PART :Not enough parameters");
        return ;
    }

    // Refuse channel lists, only one channel at a time
    if (refuseChannelList(clientData, tokens[1], "part"))
        return ;

    // Remove the # at index 0 from the channel name if there is any
    if (!tokens[1].empty() && tokens[1][0] == '#')
            tokens[1].erase(0, 1);

    const std::string channelTarget = "#" + tokens[1];

    // The reason is optional
    const std::string reason = joinTrailing(tokens, 2);

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

    // Announce the departure to every member, leaving user included, before removing him
    std::string partMsg = userPrefix(clientData) + " PART " + channelTarget;
    if (!reason.empty())
        partMsg += " :" + reason;
    broadcastIrcLine(server, targetChannel.members, partMsg);
    logMsg('i', std::string (clientData.username + " left the " + tokens[1] + " channel"), reason);

    // targetChannel must not be used after this call, the channel is deleted if it's now empty
    removeSocketFromChannel(server, tokens[1], clientData.poll.fd);
}

// Send one 352 line describing the client (H == here, @ == channel operator)
static void sendWhoReply(ClientData &clientData, const ClientData &target, const std::string &channelTarget, bool isOp){

    // 352 == RPL_WHOREPLY: <channel> <user> <host> <server> <nick> <flags> :<hopcount> <realname>
    sendNumeric(clientData, "352", channelTarget + " " + target.user + " localhost localhost " + target.username + " H" + (isOp ? "@" : "") + " :0 " + target.name);
}

// List the users of a channel, or a single user: WHO <#channel|nick>
static void handleWHO (server &server, ClientData &clientData, std::vector<std::string> tokens){

    // Check for client registration (451 == ERR_NOTREGISTERED)
    if (!clientData.isRegistered){
        sendNumeric(clientData, "451", ":You have not registered");
        return ;
    }

    // Without a mask, nothing is listed
    const std::string mask = tokens.size() < 2 ? "*" : tokens[1];

    // Channel: list every member, an unknown channel just gets an empty list
    if (mask[0] == '#')
    {
        Channel &targetChannel = server.getChannelInfo(mask.substr(1));
        for (size_t i = 0; i < targetChannel.members.size(); ++i){
            int memberSocket = targetChannel.members[i];
            const ClientData *member = server.getClient(memberSocket);
            if (member)
                sendWhoReply(clientData, *member, mask, server.isOperator(targetChannel, memberSocket));
        }
    }

    // Nickname: list that user if he exists
    else if (mask != "*")
    {
        const ClientData *target = server.getClient(server.getContactSocket(mask));
        if (target)
            sendWhoReply(clientData, *target, "*", false);
    }

    // 315 == RPL_ENDOFWHO
    sendNumeric(clientData, "315", mask + " :End of WHO list");
}

// Convert a number to a string (std::to_string is C++11)
static std::string numberToString(size_t number){

    std::ostringstream oss;
    oss << number;
    return (oss.str());
}

// Build the current channel modes and their parameters (ex: "+ikl secret 10"), the key is only shown to members
static std::string channelModes(const Channel &channel, bool showKey){

    std::string modes = "+";
    std::string params;

    if (channel.inviteOnly)
        modes += 'i';
    if (channel.restrictedTopic)
        modes += 't';
    if (channel.isPasswordLocked){
        modes += 'k';
        params += " " + (showKey ? channel.password : std::string("*"));
    }
    if (channel.userLimit){
        modes += 'l';
        params += " " + numberToString(channel.maxUsers);
    }
    return (modes + params);
}

// View or change the modes of a channel: MODE <#channel> [<+|-><mode> [param]], one mode at a time
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
        sendNumeric(clientData, "324", channelTarget + " " + channelModes(currentChannel, isMember(currentChannel, clientData.poll.fd)));
        return ;
    }

    // Ban list query, irssi sends it after joining. Bans aren't handled so the list is always empty (368 == RPL_ENDOFBANLIST)
    if (tokens.size() == 3 && (tokens[2] == "b" || tokens[2] == "+b")){
        sendNumeric(clientData, "368", channelTarget + " :End of channel ban list");
        return ;
    }

    // Check that the user is a channel operator (482 == ERR_CHANOPRIVSNEEDED)
    if (!server.isOperator(currentChannel, clientData.poll.fd)){
        logMsg('e', std::string (clientData.username + " tried " + tokens[2] + " on channel " + tokens[1] + " but isn't an operator"), "");
        sendNumeric(clientData, "482", channelTarget + " :You're not channel operator");
        return ;
    }

    // Only one mode at a time: a sign followed by a single mode letter (ex: "+i", "-k", "+l 10") (472 == ERR_UNKNOWNMODE)
    const std::string &modeString = tokens[2];
    if (modeString.size() != 2 || (modeString[0] != '+' && modeString[0] != '-')){
        logMsg('e', std::string (clientData.username + " tried MODE " + modeString + ": only one mode at a time"), "");
        sendNumeric(clientData, "472", modeString + " :is unknown mode char to me, only one mode can be changed at a time");
        return ;
    }

    const bool adding = (modeString[0] == '+');
    const char mode = modeString[1];

    // k, o and l take their parameter from tokens[3]
    const bool hasParam = (tokens.size() > 3 && !tokens[3].empty());
    const std::string param = hasParam ? tokens[3] : "";

    // Parameter added to the broadcast MODE line, if any
    std::string appliedParam;

    // Invite only, IRC servers silently ignore modes that are arleady set
    if (mode == 'i')
    {
        if (currentChannel.inviteOnly == adding)
            return ;
        currentChannel.inviteOnly = adding;
    }

    // Topic can only be changed by operators
    else if (mode == 't')
    {
        if (currentChannel.restrictedTopic == adding)
            return ;
        currentChannel.restrictedTopic = adding;
    }

    // Password protection
    else if (mode == 'k')
    {
        if (adding)
        {
            // A password must be provided (461 == ERR_NEEDMOREPARAMS)
            if (!hasParam){
                sendNumeric(clientData, "461", "MODE :Not enough parameters");
                return ;
            }

            // The password must be removed before setting a new one (467 == ERR_KEYSET)
            if (currentChannel.isPasswordLocked){
                sendNumeric(clientData, "467", channelTarget + " :Channel key already set");
                return ;
            }

            currentChannel.isPasswordLocked = true;
            currentChannel.password = param;
            appliedParam = param;
        }
        else
        {
            // Clients usually send the key with -k, it isn't needed
            if (!currentChannel.isPasswordLocked)
                return ;

            currentChannel.isPasswordLocked = false;
            currentChannel.password = "";
            appliedParam = "*";
        }
    }

    // Give or take operator privileges
    else if (mode == 'o')
    {
        // A nickname must be provided (461 == ERR_NEEDMOREPARAMS)
        if (!hasParam){
            sendNumeric(clientData, "461", "MODE :Not enough parameters");
            return ;
        }

        // The user must exist (401 == ERR_NOSUCHNICK) and be in the channel (441 == ERR_USERNOTINCHANNEL)
        int targetSocket = server.getContactSocket(param);
        if (targetSocket == -1){
            sendNumeric(clientData, "401", param + " :No such nick/channel");
            return ;
        }
        if (!isMember(currentChannel, targetSocket)){
            sendNumeric(clientData, "441", param + " " + channelTarget + " :They aren't on that channel");
            return ;
        }

        if (server.isOperator(currentChannel, targetSocket) == adding)
            return ;

        if (adding)
            currentChannel.operators.push_back(targetSocket);
        else
            removeOperator(currentChannel, targetSocket);
        appliedParam = param;
    }

    // User limit
    else if (mode == 'l')
    {
        if (adding)
        {
            // A limit must be provided (461 == ERR_NEEDMOREPARAMS)
            if (!hasParam){
                sendNumeric(clientData, "461", "MODE :Not enough parameters");
                return ;
            }

            // The limit must be a positive number, invalid limits are ignored
            char *end;
            long limit = std::strtol(param.c_str(), &end, 10);
            if (end == param.c_str() || *end != '\0' || limit <= 0 || limit > INT_MAX){
                logMsg('e', std::string (clientData.username + " tried +l on channel " + tokens[1] + " with an invalid limit:"), param);
                return ;
            }

            if (currentChannel.userLimit && currentChannel.maxUsers == static_cast<size_t>(limit))
                return ;

            currentChannel.userLimit = true;
            currentChannel.maxUsers = static_cast<size_t>(limit);
            appliedParam = numberToString(currentChannel.maxUsers);
        }
        else
        {
            if (!currentChannel.userLimit)
                return ;
            currentChannel.userLimit = false;
        }
    }

    // Unknown mode (472 == ERR_UNKNOWNMODE)
    else
    {
        logMsg('e', std::string (clientData.username + " tried MODE " + modeString + ": unknown parameter"), "");
        sendNumeric(clientData, "472", std::string(1, mode) + " :is unknown mode char to me");
        return ;
    }

    // The change is announced to all the channel members, sender included
    std::string modeChange = modeString;
    if (!appliedParam.empty())
        modeChange += " " + appliedParam;

    logMsg('i', std::string ("Channel " + tokens[1] + " mode changed by " + clientData.username + ":"), modeChange);
    broadcastIrcLine(server, currentChannel.members, userPrefix(clientData) + " MODE " + channelTarget + " " + modeChange);
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

// Leave the server: QUIT [:reason]
static void handleQUIT(server &server, ClientData &clientData, const std::vector<std::string> &tokens){

    // IRC servers use "Client Quit" when no reason is given
    const std::string message = joinTrailing(tokens, 1);
    const std::string reason = message.empty() ? "Client Quit" : "Quit: " + message;

    // Announce it to the channels now, then close the connection once the ERROR line has been sent
    quitChannels(server, clientData, reason);
    sendIrcFormattedMsg(clientData, "ERROR :Closing Link: localhost (" + reason + ")");
    clientData.quitReason = reason;
    clientData.closing = true;
    logMsg('i', std::string (clientData.username + " has quit:"), reason);
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
        // Commands are case insensitive ("join" == "JOIN")
        for (size_t i = 0; i < tokens[0].size(); i++)
            tokens[0][i] = std::toupper(static_cast<unsigned char>(tokens[0][i]));

        if (tokens[0] == "CAP")
            handleCAP(clientData, tokens);
        else if (tokens[0] == "PASS")
            handlePASS(server, clientData, tokens);        
        else if (tokens[0] == "NICK")
            handleNICK(server, clientData, tokens);
        else if (tokens[0] == "USER")
            handleUSER(clientData, tokens);
        else if (tokens[0] == "QUIT")
            handleQUIT(server, clientData, tokens);
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
        else if (tokens[0] == "PART")
            handlePART(server, clientData, tokens);
        else if (tokens[0] == "WHO")
            handleWHO(server, clientData, tokens);
        else if (tokens[0] == "MODE")
            handleMODE(server, clientData, tokens);
        else if (tokens[0] == "PING")
            handlePING(clientData, tokens);

        // Answer to a PING, the server never sends any but clients may send it to keep the connection alive
        else if (tokens[0] == "PONG")
            return ;

        // 421 == ERR_UNKNOWNCOMMAND
        else {
            logMsg('w', "Unknown command: ", tokens[0]);
            sendNumeric(clientData, "421", tokens[0] + " :Unknown command");
        }
    }
}