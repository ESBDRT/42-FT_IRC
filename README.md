*This project has been created as part of the 42 curriculum by edrouet and cllegend.*

# ft_irc

## Description

`ft_irc` is a 42 project to build an Internet Relay Chat (IRC) server in C++98. The server accepts TCP client connections and provides the core IRC features required by the subject: client authentication and registration, nicknames, channels, private and channel messages, channel operators, and operator commands and modes.

The project implements a server only. It does not implement an IRC client or server-to-server communication. The server is intended to handle multiple clients concurrently through nonblocking I/O and one readiness-multiplexing loop (`poll()` or an equivalent mechanism).

## Instructions

### Requirements

- A Linux or macOS machine
- `make` and a C++ compiler invoked as `c++` (the code is C++98, compiled with `-Wall -Wextra -Werror -std=c++98`)
- An IRC client to connect: the reference client is [irssi](https://irssi.org). `nc` (netcat) is handy to send raw commands.

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

### Run

```sh
./ircserv <port> <password>
./ircserv 6667 secret
```

- `port`: the TCP port to listen on (1-65535)
- `password`: the connection password every client must send with `PASS`

Stop the server with `Ctrl-C` (or `SIGTERM`/`SIGQUIT`): it closes every connection and exits.

## Usage

### Connecting

**With irssi (reference client)**

```sh
irssi -n alice                       # start irssi with the nickname alice
```
```
/connect localhost 6667 secret       # irssi sends PASS, NICK and USER for you
```


### Commands

#### Authentication and registration

| Raw command | irssi | Description |
|---|---|---|
| `PASS <password>` | `/connect <host> <port> <password>` | Must match the server password. Required before registration completes. |
| `NICK <nickname>` | `irssi -n <nick>`, `/nick <nick>` | Sets your nickname, or changes it later. |
| `USER <username> 0 * :<realname>` | sent by `/connect` (`/set user_name`, `/set real_name`) | Sets your username and real name. Can only be sent once. |

- Registration completes, with `001`, once all three have been received.
- Before that, channel and message commands (`JOIN`, `PART`, `PRIVMSG`, `KICK`, `INVITE`, `TOPIC`, `MODE`, `WHO`) get `451 You have not registered`.
- A nickname change after registration is announced as `:old!user@localhost NICK :new` to you and to everyone sharing a channel with you.
- Errors:
  - `461` missing parameter;
  - `431` no nickname given;
  - `433` nickname already in use;
  - `462` `USER` sent twice.

#### Channels

| Raw command | irssi | Description |
|---|---|---|
| `JOIN <#channel> [key]` | `/join #channel [key]` | Joins the channel, creates it if it doesn't exist. |
| `PART <#channel> [:reason]` | `/part #channel [reason]` | Leaves the channel. |
| `WHO <#channel>` | `/who #channel` | Lists the members of a channel. |

- The creator of a channel becomes its **operator** (shown as `@nick`); everyone else is a regular user.
- On join you receive the topic (`332`) and the member list (`353`/`366`).
- The other members see `:nick!user@localhost JOIN #channel`.
- Only one channel at a time: `JOIN #a,#b` is refused with `476`, and so is `PART #a,#b`.
- An empty channel is deleted.
- `JOIN` errors:
  - `475` wrong or missing key (`+k`);
  - `473` invite only (`+i`);
  - `471` channel full (`+l`).

#### Messages

| Raw command | irssi | Description |
|---|---|---|
| `PRIVMSG <nick> :<text>` | `/msg <nick> <text>` | Private message to a user. |
| `PRIVMSG <#channel> :<text>` | `/msg #channel <text>`, or type in the channel window | Message to every other member of the channel. |

- Only channel members can talk in a channel.
- Errors:
  - `404` not a member;
  - `401` unknown nick;
  - `403` unknown channel;
  - `412` no text.

#### Channel operator commands

All of them require being an operator of the channel. Otherwise they return `482 You're not channel operator`, except `TOPIC` on a channel without `+t`, and `INVITE` on a channel without `+i`.

| Raw command | irssi | Description |
|---|---|---|
| `KICK <#channel> <nick> [:reason]` | `/kick #channel <nick> [reason]` | Ejects a user from the channel. Everyone, including the kicked user, sees `KICK`. |
| `INVITE <nick> <#channel>` | `/invite <nick> #channel` | Invites a user. The inviter gets `341` and the user gets `INVITE`. The invite lets them join an invite-only channel once. |
| `TOPIC <#channel>` | `/topic` (in the channel window) | Shows the topic (`332`), or `331` if none is set. |
| `TOPIC <#channel> :<topic>` | `/topic <topic>` | Changes the topic. Only operators can change it when the channel is `+t`. `TOPIC #channel :` clears it. |
| `MODE <#channel>` | `/mode #channel` | Shows the current channel modes (`324`). |
| `MODE <#channel> <modes> [params]` | `/mode #channel <modes> [params]` | Changes the channel modes, see below. |

Errors:
- `461` missing parameter;
- `403` unknown channel;
- `442` you're not on that channel;
- `441` target user not on the channel;
- `443` invited user is already on the channel;
- `401` unknown nick.

#### Channel modes

| Mode | Raw command | irssi shortcut | Effect |
|---|---|---|---|
| `i` | `MODE #chan +i` / `-i` | `/mode #chan +i` | Invite only: `JOIN` requires an `INVITE`. |
| `t` | `MODE #chan +t` / `-t` | `/mode #chan +t` | Only operators can change the topic. |
| `k` | `MODE #chan +k <key>` / `-k` | `/mode #chan +k <key>` | Channel key (password): `JOIN #chan <key>` is required. |
| `o` | `MODE #chan +o <nick>` / `-o <nick>` | `/op <nick>` / `/deop <nick>` | Gives or takes channel operator privilege. |
| `l` | `MODE #chan +l <limit>` / `-l` | `/mode #chan +l <limit>` | Maximum number of members. |

- One mode at a time: `MODE #chan +i`, then `MODE #chan +k secret`. A mode string with several modes, like `+it` or `+o-t`, is refused with `472`.
- Every change is announced to all the members as `:op!user@localhost MODE #chan <changes>`.
- Errors:
  - `472` unknown mode;
  - `467` key already set (remove it with `-k` first);
  - `461` missing parameter.
- An invalid `+l` value (not a positive number) is ignored.

#### Other commands

| Raw command | irssi | Description |
|---|---|---|
| `QUIT [:reason]` | `/quit [reason]` | Disconnects. The members of your channels see `:nick!user@localhost QUIT :Quit: reason`, and you get `ERROR` before the connection closes. |
| `PING <token>` | `/quote PING hello` | The server answers `PONG localhost :<token>`. |

- Unknown commands are answered with `421 <command> :Unknown command`.
- Lines are limited to 512 bytes (`\r\n` included). Longer lines are dropped with `417`.

## Resources

### AI usage

- Extracting and organizing the project requirements from the subject
- Learning purposes
- Troubleshooting and testing
- Creating documentation as .md files