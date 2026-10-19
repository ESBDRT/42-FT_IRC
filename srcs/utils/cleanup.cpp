#include "../../include/main.hpp"

void closeAll(std::vector<struct pollfd> &clients){
    
    for (size_t i = 0; i < clients.size(); i++){
        if (clients[i].fd >= 0)
            close(clients[i].fd);
    }
}