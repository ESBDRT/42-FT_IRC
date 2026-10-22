#include "../../include/main.hpp"

static std::string displayTime(){

    char time[15];

    std::time_t now = std::time(NULL);
    std::tm* localTime = std::localtime(&now);

    if (!localTime || std::strftime(time, sizeof(time), "[%H:%M:%S]: ", localTime) == 0)
        return "";

    return time;
}

void logUserMsg(std::string user, std::string msg){
    std::cout << "[" << CLR_GREEN << user << CLR_RST << "]" << displayTime() << msg << std::endl; 
}

void logMsg(char lvl, std::string msg, std::string arg){
    
    const char *CLR;
    const char *LEVEL;

    switch (lvl) {
        case 'i':
            CLR = CLR_BLUE;
            LEVEL = "INFO";
            break;
        case 'w':
            CLR = CLR_YELLOW;
            LEVEL = "WARNING";
            break;
        case 'e':
            CLR = CLR_RED;
            LEVEL = "ERROR";
            break;
        case 'c':
            CLR = CLR_GREEN;
            LEVEL = "CLIENT";
            break;
        default:
            CLR = CLR_RST;
            LEVEL = "LOG";
            break;
    }

    std::ostream& out = (lvl == 'e') ? std::cerr : std::cout;

    out << "[" << CLR << LEVEL << CLR_RST << "]" << displayTime() << msg << CLR_RST;

    if (!arg.empty())
        out << " " << CLR << arg << CLR_RST;

    out << std::endl;
}

void display_err(std::string err){
    std::cerr << CLR_RED << "[ERROR]: " <<  CLR_RST << err << CLR_RST << std::endl;
}