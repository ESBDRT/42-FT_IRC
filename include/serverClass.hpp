#pragma once

#include <poll.h>
#include <string>
#include <vector>

class ClientData {
    public:
        ClientData();
        ClientData(const ClientData &other);
        ClientData &operator=(const ClientData &other);
        ~ClientData();

        struct pollfd   poll;
        std::string     buffer;     // Received data waiting for a \r\n
        std::string     outBuffer;  // Data waiting to be sent, flushed by run() when poll() reports POLLOUT
        std::string     name;       // Realname, set by USER
        std::string     username;   // Nickname, set by NICK
        std::string     user;       // Username, set by USER (used in the nick!user@host prefix)
        bool            hasName;
        bool            hasUsername;
        bool            passwordAccepted;
        bool            isRegistered;
        bool            discardUntilEol;    // An input line was too long, drop the rest of it until the next \r\n
        bool            closing;            // Disconnect the client once outBuffer has been sent
        std::string     quitReason;         // Reason announced to the channels when a closing client is disconnected
};

class Channel {
    public:
        Channel();
        Channel(const Channel &other);
        Channel &operator=(const Channel &other);
        ~Channel();

        std::vector<int> members;
        std::vector<int> operators;
        std::vector<int> invited;   // Sockets allowed to join while the channel is invite only
        std::string      name;
        std::string      topic;
        std::string      password;
        bool             isPasswordLocked;
        bool             restrictedTopic;
        bool             inviteOnly;
        bool             userLimit;
        size_t           maxUsers;
        size_t           currentUsers;
};

class server {
    private:
        std::string _password;
        std::vector<ClientData> _clients;
        std::vector<Channel> _channels;

        void closeAll();
        void disconnectClient(size_t index, const std::string &reason);
        void disconnectClosingClients();
        server(const server &other);
        server &operator=(const server &other);

    public:
        server();
        ~server();

        void setPassword(std::string input);
        void addNewChannel(Channel newChannel);

        int  getContactSocket(std::string contactUsername) const;
        std::string getNickname(int clientSocket) const;
        const ClientData *getClient(int clientSocket) const;
        ClientData *getClient(int clientSocket);
        void queueMsg(int clientSocket, const std::string &msg);

        std::vector<int> getChannelMembers(std::string channelName) const;
        std::vector<Channel> &getChannelsList();
        Channel &getChannelInfo(std::string channelName);

        bool checkPassword(std::string input) const;
        bool initialize(const char *str_port, const std::string &password);
        bool isOperator(Channel &channel, int &clientSocket) const;
        bool run();
};
