#include "../../include/main.hpp"

int check_port_conversion(const char *str_port){

    char *end;
    errno = 0;

    long port = strtol(str_port, &end, 10);

    if (end == str_port || *end != '\0')
        return (display_err("bad arguments: port must be only digits"), -1);
    
    if (errno == ERANGE)
        return (display_err("bad arguments: overflow/underflow"), -1);

    if (port < 1 || port > 65535)
        return (display_err("bad arguments: invalid TCP port range (must be 1-65535)"), -1);

    port = static_cast<int>(port);
    return (port);
}