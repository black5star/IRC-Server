#pragma once

#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>


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

class Server{
    public :
    Server();
    ~Server();
    Socket *sock;
    static const int MAX_CLIENT = 1024;
    static const int LARG_NUMBER = 5000;
    std::vector<Client> clt;
    std::map<std::string, std::vector<int> > channels;
};

