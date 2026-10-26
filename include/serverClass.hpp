#pragma once

#include <poll.h>
#include <string>
#include <vector>

typedef struct s_clientData {
    struct pollfd   poll;
    std::string     buffer;
    std::string     name;
    std::string     username;
    bool            hasName;
    bool            hasUsername;
    bool            passwordAccepted;
    bool            isRegistered;
}               t_clientData;

typedef struct s_channels {
    std::vector<int> members;
    std::vector<int> operators;
    std::string              name;
    std::string              topic;
    std::string              password;
    bool                     isPasswordLocked;
    bool                     restrictedTopic;
    bool                     inviteOnly;
    bool                     userLimit;
    size_t                   maxUsers;
    size_t                   currentUsers;

}               t_channel;

class server {
    private:
        std::string _password;
        std::vector<t_clientData> _clients;
        std::vector<t_channel> _channels;

        void closeAll();
        server(const server &other);
        server &operator=(const server &other);

    public:
        server();
        ~server();

        void setPassword(std::string input);
        void addNewChannel(t_channel newChannel);

        int  getContactSocket(std::string contactUsername) const;

        std::vector<int> getChannelMembers(std::string channelName) const;
        std::vector<t_channel> &getChannelsList();
        t_channel &getChannelInfo(std::string channelName);

        bool checkPassword(std::string input) const;
        bool initialize(const char *str_port, const std::string &password);
        bool isOperator(t_channel &channel, int &clientSocket) const;
        bool run();
};
