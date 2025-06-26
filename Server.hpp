#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>

#define MAX_CLIENT 1024
#define LARG_NUMBER 5000

struct Socket{
    std::string password;
    short port;
    int sockfd;
    struct sockaddr_in server_addr;       
};

struct Client{
    int client_fd;
    std::string buff;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    std::string  username;
    std::string  nickname;
    std::set<std::string> joined;
    bool        auth;
};
struct Channel {
    std::string name;
    bool invite_only;
    bool topic_restricted;
    std::string key;
    int user_limit;
    std::set<int> operators;
    std::set<int> members;
    std::set<int> invited;
    std::string topic;
};

class Server{
    public :
    Server();
    ~Server();
    Socket *sock;
    int epfd;
    std::vector<Client> clt;
    std::map<std::string, Channel> channels;
};


