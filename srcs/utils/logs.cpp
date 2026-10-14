#include "../../include/main.hpp"

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