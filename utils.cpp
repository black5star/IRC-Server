#include "Server.hpp"

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

void    RejectAndInform(int fd, Channel &ch, std::string msg){
    ch.members.erase(fd);
    ch.operators.erase(fd);
    ch.invited.erase(fd);
    for (std::set<int>::iterator it = ch.members.begin(); it != ch.members.end(); ++it){
        send(*it, msg.c_str(), msg.length(), 0);
    }

}

bool    GoToDefault(Channel chan, Client *clt){
    if (IsOperator(chan, clt) == true){
        chan.invite_only = false;
        chan.topic_restricted = false;
        chan.user_limit = -1;
        std::cout << "\nset modes to default" <<std::endl;
        return true;
    }
    return false;
}
