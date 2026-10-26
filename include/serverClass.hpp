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
        std::string     buffer;
        std::string     name;
        std::string     username;
        bool            hasName;
        bool            hasUsername;
        bool            passwordAccepted;
        bool            isRegistered;
};

class Channel {
    public:
        Channel();
        Channel(const Channel &other);
        Channel &operator=(const Channel &other);
        ~Channel();

        std::vector<int> members;
        std::vector<int> operators;
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
        server(const server &other);
        server &operator=(const server &other);

    public:
        server();
        ~server();

        void setPassword(std::string input);
        void addNewChannel(Channel newChannel);

        int  getContactSocket(std::string contactUsername) const;

        std::vector<int> getChannelMembers(std::string channelName) const;
        std::vector<Channel> &getChannelsList();
        Channel &getChannelInfo(std::string channelName);

        bool checkPassword(std::string input) const;
        bool initialize(const char *str_port, const std::string &password);
        bool isOperator(Channel &channel, int &clientSocket) const;
        bool run();
};
