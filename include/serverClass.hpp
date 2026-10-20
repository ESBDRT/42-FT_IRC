#pragma once

#include <string>

class server {
    private:
        std::string _password;
    public:
        server();
        ~server();

        void setPassword(std::string input);
        bool checkPassword(std::string input);
};
