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

bool    check(int var, std::string message);
void    CreateSocket(Server *data, const char *s1, std::string s2);
bool    AcceptNew(Server *data);
bool    HandleBuffer(std::string buffer, int fd, Server *data);
Client  *FindClient(int fd, Server * data);
bool    RecvNew(epoll_event ev, Server *data);
bool    is_auth(Client *cli);
void    handler(int signal);
bool    HandleChannels(std::string joind, Server *data, Client *clt);
bool    SendToChannel(std::string target, std::string msg, Client *clt, Server *data);
bool    SendToUser(std::string target, std::string msg, Client *clt, Server *data);
bool    HandlePrivateMsg(std::string line, Server *data, Client *clt);
bool    HandleModes(std::string str, Channel &chan, Client *clt, Server *data);
bool    HandleTopic(std::string line, Client *clt, Server *data);
bool    InviteUserToChannel(std::string line, Server *data, Client *clt);
bool    HandleChannelModes(std::string line, Server *data, Client *clt);
bool    KickUserFromChannel(std::string line, Server *data, Client *clt);
bool    IsOperator(Channel ch, Client *clt);
void    BrodcastForMode(Channel &chan, std::string msg);
bool    GoToDefault(Channel chan, Client *clt);
bool    TakePrivilege(Channel &chan, Server *data, std::string arg);
bool    GivePrivilege(Channel &chan, Server *data, std::string arg);
void    RejectAndInform(int fd, Channel &ch, std::string msg);
bool    SetTopic(std::string topic, Channel &chan, Client *clt);
bool    HandleDoubleNick(Server *data, std::string new_nick);
bool    ClearAfterDisconnection(int client_fd, Server *data);
std::pair<std::string, std::string>     ParseChannel(std::string joind);