#include "Server.hpp"
Server *g_data;

bool    KickUserFromChannel(std::string line, Server *data, Client *clt){
    size_t pos = line.find(" ", 0);
    std::string channel = line.substr(0, pos), User, reason = "", msg;
    line = line.substr(pos + 1, line.length() - (pos + 1));
    pos = line.find(" ", 0);
    if (pos == std::string::npos){
        User = line;
    }else{
        User = line.substr(0, pos);
        reason = line.substr(pos + 1, line.length() - (pos + 1));
    }
    std::map<std::string, Channel>::iterator chan = data->channels.find(channel);
    if(chan == data->channels.end()){
        std::string err = ":irc.leet.ma 403 " + clt->nickname + " " + channel + " :No such channel";
        send(clt->client_fd, err.c_str(), err.length(), 0);
        return false;
    }
    Channel &ch = chan->second;
    if (IsOperator(ch, clt) == false){
        std::string err = ":irc.leet.ma 482 " + clt->nickname + " " + channel + " :You're not channel operator\r\n";
        send(clt->client_fd, err.c_str(), err.length(), 0);
        return false;
    }
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it){
        if (it->nickname == User){
            if (it->joined.find(channel) != it->joined.end()){
                it->joined.erase(channel);
                msg = ":" + clt->nickname + "!" + clt->username + "@localhost KICK " + ch.name + " " + it->nickname;
                if (!reason.empty())
                    msg += " :" + reason;
                RejectAndInform(it->client_fd, ch, msg + "\r\n");
                return true;
            }
        }
    }
    std::string err = " :irc.leet.ma 441 " + clt->nickname + " " + User + " " + channel + " :They aren't on that channel\r\n";
    send(clt->client_fd, err.c_str(), err.length(), 0);
    return false;
}
bool    InviteUserToChannel(std::string line, Server *data, Client *clt){
    size_t pos = line.find(" ", 0);
    if (pos == std::string::npos){
        std::cout << "channel name is missing\n";
        return false;
    }
    std::string User = line.substr(0, pos);
    std::string channel = line.substr(pos + 1, line.length() - (pos + 1));
    std::map<std::string, Channel>::iterator chan = data->channels.find(channel);
    if(chan == data->channels.end()){
        std::string msg = ":irc.leet.ma 403 " + clt->nickname + " " + channel + " :No such channel\r\n";
        send(clt->client_fd, msg.c_str(), msg.length(), 0);
        return false;
    }
    Channel &ch = chan->second;
    if (IsOperator(ch, clt) == false && ch.invite_only == true){
        std::string msg = ":irc.leet.ma 482 " + clt->nickname + " " + channel + " :You're not channel operator\r\n";
        send(clt->client_fd, msg.c_str(), msg.length(), 0);
        return false;
    }
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it){
        if (it->nickname == User){
            ch.invited.insert(it->client_fd);
            std::string msg = ":irc.leet.ma 341 " + clt->nickname + " " + it->nickname + " " + ch.name + "\r\n";
            send(clt->client_fd, msg.c_str(), msg.length(), 0);
            msg = ":" + clt->nickname + "!" + clt->username + "@localhost INVITE " + it->nickname + " :" + ch.name + "\r\n";
            send(it->client_fd, msg.c_str(), msg.length(), 0);
            return true;
        }
    }
    std::string msg = ":irc.leet.ma 401 " + clt->nickname + " " + User + ":No such nick/channel\r\n";
    send(clt->client_fd, msg.c_str(), msg.length(), 0);
    return false;
}

bool    HandleChannelModes(std::string line, Server *data, Client *clt){
    size_t pos = line.find(" ", 0);
    std::string ch_name, mode;
    if(pos == std::string::npos){
        ch_name = line;
    }else{
        ch_name = line.substr(0, pos);
        mode = line.substr(pos + 1, line.length() - (pos + 1));
    }
    std::map<std::string, Channel>::iterator it = data->channels.find(ch_name);
    if (it == data->channels.end()) {
        std::string err = ":irc.leet.ma 403 " + clt->nickname + " " + ch_name + " :No such channel\r\n";
        send(clt->client_fd, err.c_str(), err.length(), 0);
        return false;
    }
    if (it->second.members.size() == 1 && mode.empty()){
        GoToDefault(it->second, clt);
        return true;
    }
    if (IsOperator(it->second, clt) == true){
        HandleModes(mode, it->second, clt, data);
    }
    else {
        std::string err = ":irc.leet.ma 482 " + clt->nickname + " " + ch_name + " :You're not channel operator\r\n";
        send(clt->client_fd, err.c_str(), err.length(), 0);
    }
    return true;
}

void    handler(int signal){
    std::cout << "\nShut down the server\n";
    if (signal == SIGINT){
        for (std::vector<Client>::iterator it = g_data->clt.begin(); it != g_data->clt.end(); ++it) {
            close(it->client_fd);
        }
        g_data->clt.clear();
        g_data->channels.clear();
        delete g_data;
        exit(EXIT_SUCCESS);
    }
}

int main(int ac, char **av)
{
    if (ac != 3){
        std::cerr << "wrong args passed.\n";
        exit(EXIT_FAILURE);
    }
    Server *data = new Server;
    struct epoll_event ep;
    data->epfd = epoll_create1(0);
    if(check(data->epfd, "epoll_create failed") == false){
        delete data;
        return 1;
    }
    g_data = data;
    signal(SIGINT, handler);
    
    CreateSocket(data, av[1], av[2]);
    ep.events = EPOLLIN;
    ep.data.fd = data->sock->sockfd;
    if(check(epoll_ctl(data->epfd, EPOLL_CTL_ADD, data->sock->sockfd, &ep), "epoll_ctl failed") == false){
        delete data;
        return 1;
    }
    while (true)
    {
        struct epoll_event ev[MAX_CLIENT];
        int nfds = epoll_wait(data->epfd, ev, MAX_CLIENT, -1);
        for(int i = 0; i < nfds; ++i){
            if (ev[i].data.fd == data->sock->sockfd)
            {
                if(AcceptNew(data) == false)
                    continue ;
                
            } else {
                RecvNew(ev[i], data);
            }
        }
    }
    delete data;
}
