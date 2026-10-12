#include "../../include/main.hpp"

bool check_port_conversion(const char *str_port, int &port){

    char *end;
    errno = 0;

    long n = strtol(str_port, &end, 10);

    if (end == str_port || *end != '\0')
        return (display_err(": bad arguments: port must be only digits"), false);
    
    if (errno == ERANGE)
        return (display_err(": bad arguments: overflow/underflow"), false);

    if (n < 1 || n > 65535)
        return (display_err(": bad arguments: invalid TCP port range (must be 1-65535)"), false);

    port = static_cast<int>(n);
    return (true);
}