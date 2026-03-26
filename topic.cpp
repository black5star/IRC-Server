#include "Server.hpp"

bool    ViewTopic(Channel chan, Client *clt) {
    std::string format;
    if (chan.topic.empty()){
        format = ":irc.leet.ma 331 " + clt->nickname + " " + chan.name + " :No topic is set\r\n";
        send(clt->client_fd, format.c_str(), format.length(), 0);
        return true;
    }
    format = ":irc.leet.ma 332 " + clt->nickname + " " + chan.name + " :" + chan.topic + "\r\n";
    send(clt->client_fd, format.c_str(), format.length(), 0);
    return true;
}

bool    SetTopic(std::string topic, Channel &chan, Client *clt) {
    if (IsOperator(chan, clt) == false && chan.topic_restricted == true){
        std::string msg = ":irc.leet.ma 482 " + clt->nickname + " " + chan.name + " :You're not channel operator\r\n";
        return false;
    }
    chan.topic = topic;
    std::string format = ":" + clt->nickname + "!" + clt->username + "@localhost TOPIC " + chan.name + " :" + chan.topic + "\r\n";
    for(std::set<int>::iterator it = chan.members.begin(); it != chan.members.end(); ++it){
        send(*it, format.c_str(), format.length(), 0);
    }
    return true;
}

bool    HandleTopic(std::string str, Client *clt, Server *data) {
    size_t pos = str.find(" ", 0);
    std::string channel;
    if(pos == std::string::npos){
        channel = str;
    }else{
        channel = str.substr(0, pos);
        str = str.substr(pos + 1, str.length() - (pos + 1));
    }
    std::map<std::string, Channel>::iterator chan = data->channels.find(channel);
    if (chan == data->channels.end()){
        std::string err = ":irc.leet.ma 403 " + clt->nickname + " " + channel + " :No such channel\r\n";
        send(clt->client_fd, err.c_str(), err.length(), 0);
        return false;
    }
    if (str.empty()){
        ViewTopic(chan->second, clt);
    }else{
        SetTopic(str, chan->second, clt);
    }
    return true;
}