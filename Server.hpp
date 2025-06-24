#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>

int BOT;

struct Bot{
    int fd;
    std::string sever_ip;
    std::string user;
    std::string nick;
    std::string pass;
    std::string channel;
    short port;
    struct sockaddr_in bot_addr;
    socklen_t bot_len;
};

struct Socket{
    std::string password;
    short port;
    int sockfd;
    struct sockaddr_in server_addr;       
};

struct Client{
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    std::string  username;
    std::string  nickname;
    std::set<std::string> joined;
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
    static const int MAX_CLIENT = 1024;
    static const int LARG_NUMBER = 5000;
    std::vector<Client> clt;
    std::map<std::string, Channel> channels;
};

Server *g_data;
