*This project has been created as part of the 42 curriculum by edrouet, cllegend and lucinguy.*

# ft_irc

## Description

`ft_irc` is a 42 project to build an Internet Relay Chat (IRC) server in C++98. The server accepts TCP client connections and provides the core IRC features required by the subject: client authentication and registration, nicknames, channels, private and channel messages, channel operators, and operator commands and modes.

The project implements a server only. It does not implement an IRC client or server-to-server communication. The server is intended to handle multiple clients concurrently through nonblocking I/O and one readiness-multiplexing loop (`poll()` or an equivalent mechanism).

## Instructions

### Requirements

### Compile

From the repository root, build the `ircserv` executable with:

```sh
make
```

The Makefile also provides `clean`, `fclean`, and `re` targets:

```sh
make clean  # remove object files
make fclean # remove object files and the executable
make re     # clean and rebuild
```


## Resources

### AI usage

- Extracting and organizing the project requirements from the subject
- Learning purposes
- Troubleshooting 