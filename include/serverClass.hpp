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
    bool            isOperator;
}               t_clientData;

class server {
    private:
        std::string _password;
        std::vector<t_clientData> _clients;

        void closeAll();
        server(const server &other);
        server &operator=(const server &other);

    public:
        server();
        ~server();

        void setPassword(std::string input);
        bool checkPassword(std::string input);
        bool initialize(const char *str_port, const std::string &password);
        bool run();
};
