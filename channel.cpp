#include "Server.hpp"


std::pair<std::string, std::string> ParseChannel(std::string joind) {
    std::string channel = "", key = "";
    std::pair<std::string, std::string> res;
    res = make_pair(channel, key);
    size_t pos = joind.find("#", 0), len = 0;
    if (pos){
        return res;}
    size_t pos2 = joind.find(" ", 0);
    if (pos2 == 1)
        return res;
    if (pos2 != std::string::npos){
        key = joind.substr(pos2 + 1, joind.length() - (pos2 + 1));
        len = pos2;
    }else if(pos2 == std::string::npos){
        len = joind.length();
    }
    if (len > 50){
        len = 50;}
    channel = joind.substr(0, len);
    res = make_pair(channel, key);
    return res;
}

bool    CheckJoinningPermission(Channel &chan, std::string &key, Client *clt) {
    if (chan.invite_only){
        for(std::set<int>::iterator it = chan.invited.begin(); it != chan.invited.end(); ++it){
            if (*it == clt->client_fd){
                return true;
            }
        }
        return false;
    }
    if (chan.user_limit != -1){
        if ((int)chan.members.size() >= chan.user_limit){
            return false;
        }
    }
    if(!chan.key.empty()){
        if(chan.key != key){
            std::string err = ":irc.leet.ma 475 " + clt->nickname + " " + chan.name + " :Cannot join channel (+k) - bad key\r\n";
            send(clt->client_fd, err.c_str(), err.length(), 0);
            return false;
        }
    }
    return true;
}

bool    HandleChannels(std::string joind, Server *data, Client *clt) {
    Channel ch;
    std::pair<std::string, std::string> res = ParseChannel(joind);
    ch.name = res.first;
    std::string key = res.second;
    if (ch.name.empty()){
        std::cerr << "invalide channel name. Try /join #<channelname>\n";
        return false;
    }
    std::map<std::string, Channel>::iterator it = data->channels.find(ch.name);
    if (it == data->channels.end()){
        ch.operators.insert(clt->client_fd);
        ch.members.insert(clt->client_fd);
        ch.key = key;
        ch.invite_only = false;
        ch.topic_restricted = false;
        ch.user_limit = -1;
        data->channels.insert(std::pair<std::string, Channel>(ch.name, ch));
        clt->joined.insert(ch.name);
        std::string reform = ":" + clt->nickname + "!" + clt->username + "@localhost JOIN :" + ch.name + "\r\n";
        send(clt->client_fd, reform.c_str(), reform.length(), 0);
    }else if(CheckJoinningPermission(it->second, key, clt) == false){
        std::cout << "permission denied\n";
        return false;
    }
    else{
        Channel &chan = it->second;
        chan.members.insert(clt->client_fd);
        clt->joined.insert(ch.name);
        for (std::set<int>::iterator it = chan.members.begin(); it != chan.members.end(); ++it) {
            std::string reform = ":" + clt->nickname + "!" + clt->username + "@localhost JOIN :" + ch.name + "\r\n";
            send(*it, reform.c_str(), reform.length(), 0);
        }
    }
    return true;
}