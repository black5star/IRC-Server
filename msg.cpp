#include "Server.hpp"

bool    SendToChannel(std::string target, std::string msg, Client *clt, Server *data) {
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

bool    SendToUser(std::string target, std::string msg, Client *clt, Server *data) {
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it) {
        if (it->nickname == target) {
            Client *receiver = &(*it);
            if (receiver->client_fd == clt->client_fd)
                return true;
            std::string reform = ":" + clt->nickname + "!" + clt->username
                + "@localhost PRIVMSG " + target + msg + "\r\n";
            send(receiver->client_fd, reform.c_str(), reform.length(), 0);
            return true;
        }
    }
    return false;
}

bool    HandlePrivateMsg(std::string line, Server *data, Client *clt) {
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