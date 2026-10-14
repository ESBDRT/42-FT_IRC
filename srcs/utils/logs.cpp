#include "../../include/main.hpp"

static std::string displayTime(){

    char time[15];

    std::time_t now = std::time(NULL);
    std::tm* localTime = std::localtime(&now);

    if (!localTime || std::strftime(time, sizeof(time), "[%H:%M:%S]: ", localTime) == 0)
        return "";

    return time;
}

void logMsg(char lvl, std::string msg){

    const char *CLR;

    switch (lvl) {
        case 's': CLR = CLR_GREEN;  break; // SUCCESS
        case 'i': CLR = CLR_BLUE;   break; // INFO
        case 'w': CLR = CLR_YELLOW; break; // WARNING
        case 'e': CLR = CLR_RED;    break; // ERROR
        default:  CLR = CLR_RST;    break;
    }

    std::ostream& out = (lvl == 'e') ? std::cerr : std::cout;

    out << CLR_BLUE << "[SERVER]" << CLR_RST << displayTime() << CLR << msg << CLR_RST << std::endl;
}

void display_err(std::string err){
    std::cerr << CLR_RED << "[ERROR]: " <<  CLR_RST << err << CLR_RST << std::endl;
}