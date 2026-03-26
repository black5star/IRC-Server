#include "Server.hpp"

bool    TakePrivilege(Channel &chan, Server *data, std::string arg) {
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it){
        if (it->nickname == arg){
            chan.operators.erase(it->client_fd);
            return true;
        }
    }
    return false;
}

bool    GivePrivilege(Channel &chan, Server *data, std::string arg) {
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it){
        if (it->nickname == arg){
            chan.operators.insert(it->client_fd);
            return true;
        }
    }
    return false;
}

void    BrodcastForMode(Channel &chan, std::string msg) {
    for (std::set<int>::iterator it = chan.members.begin(); it != chan.members.end(); ++it){
        send(*it, msg.c_str(), msg.length(), 0);
    }
}

std::string SetModes(std::string flag, std::string &str, Channel &chan, Server *data) {
    std::string b_flag, b_args = "", arg;
    size_t pos;
    b_flag = "+";
    for (size_t i = 1; i < flag.length(); ++i){
        if (flag[i] == 'i' && (chan.invite_only == false)){
            chan.invite_only = true;
            b_flag = b_flag + "i";
        }
        if (flag[i] == 't' && (chan.topic_restricted == false)){
            chan.topic_restricted = true;
            b_flag = b_flag + "t";
        }
        if (flag[i] == 'l' || flag[i] == 'o' || flag[i] == 'k'){
            if (str.empty())
                break ;
            pos = str.find(" ", 0);
            if (pos == std::string::npos){
                arg = str;
            } else {
                arg = str.substr(0, pos);
                str = str.substr(pos + 1, str.length() - (pos + 1));
            }
            if (flag[i] == 'l'){
                chan.user_limit = atoi(arg.c_str());
                b_flag = b_flag + "l";
                if (!b_args.empty()){
                    b_args += " ";
                }
                b_args = b_args + arg;
            }
            if (flag[i] == 'o'){
                if (GivePrivilege(chan, data, arg) == true){
                    b_flag = b_flag + "o";
                    if (!b_args.empty()){
                        b_args += " ";
                    }
                    b_args = b_args + arg;
                }
            }
            if (flag[i] == 'k'){
                chan.key = arg;
                b_flag = b_flag + "k";
                if (!b_args.empty()){
                    b_args += " ";
                }
                b_args = b_args + arg;
            }
        }
    }
    return (b_flag + " " + b_args);
}

std::string RemoveMode(std::string flag, std::string &str, Channel &chan, Server *data) {
    std::string b_flag, b_args = "", arg;
    size_t pos;
    b_flag = "-";
    for (size_t i = 1; i < flag.length(); ++i){
        if (flag[i] == 'i' && (chan.invite_only != false)){
            chan.invite_only = false;
            b_flag = b_flag + "i";
        }
        if (flag[i] == 't' && (chan.topic_restricted != false)){
            chan.topic_restricted = false;
            b_flag = b_flag + "t";
        }
        if (flag[i] == 'k' && (chan.key != "")){
            chan.key = "";
            b_flag = b_flag + "k";
        }
        if ((flag[i] == 'l') && (chan.user_limit != -1)){
            chan.user_limit = -1;
            b_flag = b_flag + "l";
        }
        if (flag[i] == 'o'){
            if (str.empty())
                break ;
            pos = str.find(" ", 0);
            if (pos == std::string::npos){
                arg = str;
            } else {
                arg = str.substr(0, pos);
                str = str.substr(pos + 1, str.length() - (pos + 1));
            }
            if (flag[i] == 'o'){
                if (TakePrivilege(chan, data, arg) == true){
                    b_flag = b_flag + "o";
                    if (!b_args.empty()){
                        b_args += " ";
                    }
                    b_args = b_args + arg;
                }
            }
        }
    }
    return (b_flag + " " + b_args);
}

bool    HandleModes(std::string str, Channel &chan, Client *clt, Server *data) {
    std::string flag, msg = "", format;
    while (str.length() != 0){
        size_t pos = str.find(" ", 0);
        if (pos == std::string::npos){
            flag = str;
            str = "";
        } else {
            flag = str.substr(0, pos);
            str = str.substr(pos + 1, str.length() - (pos + 1));
        }
        if (flag[0] == '+'){
            if (!msg.empty()){
                msg += " ";
            }
            msg = msg + SetModes(flag, str, chan, data);
        } else if (flag[0] == '-'){
            if (!msg.empty()){
                msg += " ";
            }
            msg = msg + RemoveMode(flag, str, chan, data);
        }
    }
    format = ":" + clt->nickname + "!" + clt->username + "@localhost MODE " + chan.name + " " + msg + "\r\n";
    BrodcastForMode(chan, format);
    return true;
}