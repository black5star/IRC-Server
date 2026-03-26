# IRC Server

A fully-featured Internet Relay Chat (IRC) server implementation written in C++. This server supports multi-client connections, channel management, user authentication, and advanced IRC features.

## Features

- **Multi-client Support**: Handles up to 1024 concurrent clients using epoll for efficient I/O multiplexing
- **Channel Management**: Create, join, and manage IRC channels
- **User Authentication**: Password-based server authentication
- **Channel Modes**: Support for various channel modes including:
  - Invite-only channels (`+i`/`-i`)
  - Topic restrictions (`+t`/`-t`)
  - Key-protected channels (`+k`/`-k`)
  - User limits (`+l`/`-l`)
  - Operator privileges (`+o`/`-o`)
- **User Privileges**: Operator status with privilege management
- **Private Messaging**: Direct user-to-user communication
- **Channel Topics**: Set and manage channel topics with appropriate restrictions
- **Kick/Invite**: Channel operators can kick users and invite others
- **RFC 1459 Compliant**: Follows IRC protocol standards

## Prerequisites
c
- C++98 ompatible compiler (g++ or clang++)
- Linux OS with epoll support
- Standard C++ libraries
- Make utility

## Building

### Compilation

```bash
make
```

This will compile all source files and generate the `ircserv` executable.

### Clean

```bash
# Remove object files
make clean

# Remove object files and executable
make fclean

# Clean and rebuild
make re
```

## Usage

### Starting the Server

```bash
./ircserv <port> <password>
```

**Parameters:**
- `<port>`: The port number to listen on (e.g., 6667)
- `<password>`: Server password for client authentication

**Example:**
```bash
./ircserv 6667 mypassword
```

### Connecting to the Server

Use an IRC client like `nc`, `telnet`, or dedicated IRC clients (e.g., irssi, HexChat):

```bash
nc localhost 6667
```

## IRC Commands Supported

### Authentication
- `PASS <password>` - Set server password
- `NICK <nickname>` - Set user nickname
- `USER <username> <mode> <unused> :<realname>` - Register user

### Channel Operations
- `JOIN <channel>` - Join a channel
- `TOPIC <channel> [topic]` - View or set channel topic
- `KICK <channel> <user> [reason]` - Kick user from channel (operators only)
- `INVITE <user> <channel>` - Invite user to channel

### Messaging
- `PRIVMSG <target> :<message>` - Send private message to user or channel

### User/Channel Management
- `MODE <target> <modes> [arguments]` - Modify user or channel modes
- `QUIT [message]` - Disconnect from server

### Query
- `WHO [target]` - List users (in channel or server wide)
- `WHOIS <user>` - Get user information

## Project Structure

```
IRC-Server/
├── main.cpp           # Server entry point
├── Server.cpp         # Server implementation
├── Server.hpp         # Server header and data structures
├── socket.cpp         # Socket creation and management
├── received.cpp       # Message receiving logic
├── channel.cpp        # Channel operations
├── topic.cpp          # Topic management
├── msg.cpp            # Message handling
├── modes.cpp          # Mode management
├── utils.cpp          # Utility functions
├── Makefile           # Build configuration
└── README.md          # This file
```

## Implementation Details

- **Concurrency Model**: Uses epoll for efficient event-driven I/O handling
- **Architecture**: Single-threaded event loop with epoll multiplexing
- **Maximum Connections**: 1024 simultaneous clients
- **Protocol**: IRC (RFC 1459) compatible
- **Compiler**: C++98 standard compliance

## Compilation Flags

```makefile
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
```

These flags ensure:
- All warnings enabled (`-Wall -Wextra`)
- Warnings treated as errors (`-Werror`)
- C++98 standard compatibility (`-std=c++98`)

## Notes

- Server requires elevated permissions for ports below 1024
- Password authentication is required for all client connections
- Channel names typically start with `#` (e.g., `#general`)
- User nicknames must be unique across the server
- The server implements signal handling for graceful shutdown

## License

This project is provided as-is for educational purposes.
