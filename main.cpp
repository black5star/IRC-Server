#include "Server.hpp"

void check(int var, std::string message){
    if(var < 0){
        std::cerr << message << std::endl;
        exit(EXIT_FAILURE);
    }
}
void CreateSocket(Server *data, const char *s1, std::string s2){
    data->sock->password = (std::string)s2;
    data->sock->port = atoi(s1);

    data->sock->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    check(data->sock->sockfd, "socket failed");

    data->sock->server_addr.sin_family = AF_INET;
    data->sock->server_addr.sin_port = htons(data->sock->port);
    data->sock->server_addr.sin_addr.s_addr = INADDR_ANY;
    check(bind(data->sock->sockfd, (struct sockaddr *)&data->sock->server_addr,
        sizeof(data->sock->server_addr)), "bind failed");
    check(listen(data->sock->sockfd, 5), "listen failed");
}
bool    AcceptNew(Server *data, int epfd){
    Client new_clt;
    new_clt.client_len = sizeof(new_clt.client_addr);
    new_clt.client_fd = accept(data->sock->sockfd,
        (sockaddr *)&new_clt.client_addr, &new_clt.client_len);
    if(new_clt.client_fd < 0){
        std::cerr << "accept failed\n";
        return false;
    }
    struct epoll_event clt_ev;
    clt_ev.events = EPOLLIN;
    clt_ev.data.fd = new_clt.client_fd;
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, new_clt.client_fd, &clt_ev) < 0){
        std::cerr << "epoll_ctl failed\n";
        return false;
    }
    data->clt.push_back(new_clt);
    return true;
}
Client *FindClient(int fd, Server * data){
    
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it) {
        if (it->client_fd == fd) {
            return &(*it);
        }
    }
    return NULL;
}
std::string ParseChannel(std::string joind){
    std::string channel;
    size_t pos = joind.find("#", 0);
    if (pos){
        return channel;}
    size_t pos2 = joind.find(" ", 0);
    if (pos2 == 1)
        return channel;
    if (pos2 == std::string::npos){
        pos2 = joind.length();}
    if (pos2 > 50){
        pos2 = 50;}
    channel = joind.substr(0, pos2);
    return channel;
}
bool    HandleChannels(std::string joind, Server *data, Client *clt){
    (void)data;
    (void)clt;
    Channel ch;
    ch.name = ParseChannel(joind);
    if (ch.name.empty()){
        std::cerr << "invalide channel name. Try /join #<channelname>\n";
        return false;
    }
    std::map<std::string, Channel>::iterator it = data->channels.find(ch.name);
    if(it == data->channels.end())
        ch.operators.insert(clt->client_fd);
    ch.members.insert(clt->client_fd);
    clt->joined.insert(ch.name);
    data->channels[ch.name] = ch;
    std::string reform = ":" + clt->nickname + "!" + clt->username + "@localhost JOIN :" + ch.name + "\r\n";
    send(clt->client_fd, reform.c_str(), reform.length(), 0);
    return true;
}
bool    SendToChannel(std::string target, std::string msg, Client *clt, Server *data){
    if (data->channels.find(target) == data->channels.end())
        return false ;
    std::string reform = ":" + clt->nickname + "!" + clt->username
            + "@localhost PRIVMSG " + target + msg + "\r\n";

    Channel &ch = data->channels[target];
    
    for (std::set<int>::iterator it = ch.members.begin(); it != ch.members.end(); ++it) {
        int fd = *it;
        if(fd != clt->client_fd)
            send(fd, reform.c_str(), reform.size(), 0);
    }
    return true;
}
bool    SendToUser(std::string target, std::string msg, Client *clt, Server *data){
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it) {
        if (it->nickname == target) {
            Client *receiver = &(*it);
            std::string reform = ":" + clt->nickname + "!" + clt->username
                + "@localhost PRIVMSG " + target + msg + "\r\n";
            send(receiver->client_fd, reform.c_str(), reform.length(), 0);
            return true;
        }
    }
    return false;
}

bool    HandlePrivateMsg(std::string line, Server *data, Client *clt){
    size_t pos = line.find(" ", 0);
    std::string target = line.substr(0, pos);
    std::string msg = line.substr(pos , (line.length() - pos));
    std::set<std::string>::iterator it;
    if(target[0] == '#'){
        it = clt->joined.find(target);
        if (clt->joined.find(target) == clt->joined.end())
            return false;    
        SendToChannel(target, msg, clt, data);
    }
    else{
        SendToUser(target, msg, clt, data);
    }
    return true ;
}
bool    HandleDoubleNick(Server *data, std::string new_nick){
    
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it){
        if (it->nickname == new_nick){
            return false;
        }
    }
    return true;
}
bool    IsOperator(Channel ch, Client *clt){
    for(std::set<int>::iterator it = ch.operators.begin(); it != ch.operators.end(); ++it){
        if(*it == clt->client_fd){
            return true;
        }
    }
    return false;
}
bool    KickUserFromChannel(std::string line, Server *data, Client *clt){
    int pos = line.find(" ", 0), fd;
    std::string channel = line.substr(0, pos);
    std::string User = line.substr(pos + 1, line.length() - (pos + 1));
    Channel &ch = data->channels[channel];
    if (IsOperator(ch, clt) == false){
        std::cout << "Permission denied\n";
        return false;
    }
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it){
        if (it->nickname == User){
            it->joined.erase(channel);
            ch.members.erase(it->client_fd);
            ch.operators.erase(it->client_fd);
        }
    }
    return true;
}
bool    InviteUserToChannel(std::string line, Server *data, Client *clt){
    int pos = line.find(" :", 0);
    std::string User = line.substr(0, pos);
    std::string channel = line.substr(pos + 5, line.length() - (pos + 1));
    Channel &ch = data->channels[channel];
    if (IsOperator(ch, clt) == false){
        std::cout << "Permission denied\n";
        return false;
    }
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it){
        if (it->nickname == User){
            // send(it->client_fd, );
        }
    }
}
bool    HandleBuffer(std::string buffer, int fd, Server *data){
    Client *clt = FindClient(fd, data);
    if (clt == NULL){
        return false;}
    size_t buff_size = buffer.length(), flag = 1, pos = 0, i = 0;
    while(i < buff_size){
        pos = buffer.find("\r\n", i);
        if (pos == std::string::npos){
            pos = buffer.find("\n", i);
            flag = 0;
        }
        std::string line = buffer.substr(i, pos - i);
        if (!line.find("PASS ", 0)){
            std::string pass = line.substr(5, line.length() - 5);
            if (pass != data->sock->password){
                std::cout << "\nWrong password\n";
                return false;}
            std::cout << "\nClient connected.\n";
        }
        if (!line.find("NICK ", 0)){
            std::string new_nick = line.substr(5, line.length() - 5);
            if (HandleDoubleNick(data, new_nick) == false){
                if(clt->nickname.empty()){
                    std::cout << "\nERROR : NickName exist try with other NickName\n";
                    std::cout << "Client disconnected.\n";
                    return false;
                }
                std::cout << "\nNickName already exist\n";
            }
            else {
                std::string old_nick = clt->nickname;
                clt->nickname = new_nick;
                std::string nick_change_msg = ":" + old_nick + "!" + clt->username + "@localhost NICK :" + clt->nickname + "\r\n";
    
                for (std::set<std::string>::iterator ch = clt->joined.begin(); ch != clt->joined.end(); ++ch) {
                    Channel &chan = data->channels[*ch];
                    for (std::set<int>::iterator it = chan.members.begin(); it != chan.members.end(); ++it) {
                        if (*it != clt->client_fd) {
                            send(*it, nick_change_msg.c_str(), nick_change_msg.length(), 0);
                        }
                    }
                }
            }
        }
        if (!line.find("USER ", 0)){
            std::string temp = line.substr(5, line.length() - 5);
            int n = temp.find(" ", 0);
            clt->username = temp.substr(0, n);
        }
        if (!line.find("JOIN ", 0)){
            std::string join = line.substr(5, line.length() - 5);
            HandleChannels(join, data, clt);
        }
        if (!line.find("PRIVMSG ", 0)){
            std::string msg = line.substr(8, line.length() - 8);
            HandlePrivateMsg(msg, data, clt);
        }
        if (!line.find("KICK ", 0)){
            std::string temp = line.substr(5, line.length() - 5);
            KickUserFromChannel(temp, data, clt);
        }
        if (!line.find("INVITE ")){
            std::string temp = line.substr(7, line.length() - 7);
            InviteUserToChannel(temp, data, clt);
        }
        i = pos + 1 + flag;
    }
    return true;
}

bool    ClearAfterDisconnection(int client_fd, int epfd, Server *data){
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL) < 0) {
        std::cerr << "epoll_ctl DEL failed\n";
        return false;
    }
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it) {
        if (it->client_fd == client_fd) {
            data->clt.erase(it);
            break;
        }
    }
    return true;
}

bool    RecvNew(epoll_event ev, Server *data, int epfd)
{
    char    buffer[Server::LARG_NUMBER];
    int     client_fd = ev.data.fd;
    int rd = recv(ev.data.fd, buffer, (Server::LARG_NUMBER) - 1, 0);
    if (rd > 0){
        buffer[rd] = '\0';
        std::cout << buffer << std::endl;
        if (HandleBuffer((std::string)buffer, ev.data.fd, data) == false){
            ClearAfterDisconnection(client_fd, epfd, data);
        }
    }
    if (rd == 0) {
        std::cout << "\nClient disconnected.\n";
        ClearAfterDisconnection(client_fd, epfd, data);
    } else if (rd < 0) {
        std::cerr << "recv failed\n";
    }
    return true;
}

int main(int ac, char **av)
{
    if (ac != 3){
        std::cerr << "wrong args passed.\n";
        exit(EXIT_FAILURE);
    }
    struct epoll_event ep;

    int epfd = epoll_create1(0);
    check(epfd, "epoll_create failed");
    Server *data = new Server;
    
    CreateSocket(data, av[1], av[2]);
    ep.events = EPOLLIN;
    ep.data.fd = data->sock->sockfd;
    check(epoll_ctl(epfd, EPOLL_CTL_ADD, data->sock->sockfd, &ep), "epoll_ctl failed");
    while (true)
    {
        struct epoll_event ev[Server::MAX_CLIENT];
        int nfds = epoll_wait(epfd, ev, Server::MAX_CLIENT, -1);
        for(int i = 0; i < nfds; ++i){
            if (ev[i].data.fd == data->sock->sockfd)
            {
                if(AcceptNew(data, epfd) == false)
                    continue ;
                
            } else {
                RecvNew(ev[i], data, epfd);
            }
        }
    }
    delete data;
}
