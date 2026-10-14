#include "../../include/main.hpp"

void displayTime(){

    char time[13];

    std::time_t now = std::time(NULL);
    std::tm *localTime = std::localtime(&now);

    if (!localTime || std::strftime(time, sizeof(time), "[%H:%M:%S]", localTime) == 0)
        return;

    std::cout << time;
}