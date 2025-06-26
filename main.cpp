#include "Server.hpp"
Server *g_data;

bool check(int var, std::string message){
    if(var < 0){
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}
void CreateSocket(Server *data, const char *s1, std::string s2){
    data->sock->password = (std::string)s2;
    data->sock->port = atoi(s1);

    data->sock->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(check(data->sock->sockfd, "socket failed") == false){
        delete data;
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(data->sock->sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt(SO_REUSEADDR) failed" << std::endl;
        delete data;
        exit(EXIT_FAILURE);
    }

    data->sock->server_addr.sin_family = AF_INET;
    data->sock->server_addr.sin_port = htons(data->sock->port);
    data->sock->server_addr.sin_addr.s_addr = INADDR_ANY;
    if(check(bind(data->sock->sockfd, (struct sockaddr *)&data->sock->server_addr,
        sizeof(data->sock->server_addr)), "bind failed") == false){
        delete data;
        exit(EXIT_FAILURE);
    }
    if(check(listen(data->sock->sockfd, 5), "listen failed") == false){
        delete data;
        exit(EXIT_FAILURE);
    }
}
bool    AcceptNew(Server *data){
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
    if(epoll_ctl(data->epfd, EPOLL_CTL_ADD, new_clt.client_fd, &clt_ev) < 0){
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
std::pair<std::string, std::string> ParseChannel(std::string joind){
    std::string channel = "", key = "";
    std::pair<std::string, std::string> res;
    res = make_pair(channel, key);
    size_t pos = joind.find("#", 0), len;
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
bool    CheckJoinningPermission(Channel &chan, std::string &key, Client *clt){
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
bool    HandleChannels(std::string joind, Server *data, Client *clt){
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
void    RejectAndInform(int fd, Channel &ch, std::string msg){
    ch.members.erase(fd);
    ch.operators.erase(fd);
    ch.invited.erase(fd);
    for (std::set<int>::iterator it = ch.members.begin(); it != ch.members.end(); ++it){
        send(*it, msg.c_str(), msg.length(), 0);
    }

}
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
bool    TakePrivilege(Channel &chan, Server *data, std::string arg){
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it){
        if (it->nickname == arg){
            chan.operators.erase(it->client_fd);
            return true;
        }
    }
    return false;
}
bool    GivePrivilege(Channel &chan, Server *data, std::string arg){
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it){
        if (it->nickname == arg){
            chan.operators.insert(it->client_fd);
            return true;
        }
    }
    return false;
}
void    BrodcastForMode(Channel &chan, std::string msg){
    for (std::set<int>::iterator it = chan.members.begin(); it != chan.members.end(); ++it){
        send(*it, msg.c_str(), msg.length(), 0);
    }
}
std::string SetModes(std::string flag, std::string &str, Channel &chan, Server *data){
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
std::string RemoveMode(std::string flag, std::string &str, Channel &chan, Server *data){
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
bool    HandleModes(std::string str, Channel &chan, Client *clt, Server *data)
{
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
bool    ViewTopic(Channel chan, Client *clt){
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
bool    SetTopic(std::string topic, Channel &chan, Client *clt){
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
bool    HandleTopic(std::string str, Client *clt, Server *data){
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

bool    ClearAfterDisconnection(int client_fd, Server *data){
    if (epoll_ctl(data->epfd, EPOLL_CTL_DEL, client_fd, NULL) < 0) {
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

bool    HandleBuffer(std::string buffer, int fd, Server *data){
    Client *clt = FindClient(fd, data);
    if (clt == NULL){
        return false;}
    size_t buff_size = buffer.length(), flag = 1, pos = 0, i = 0;
    while(i <= buff_size){
        if (i == buff_size){
            clt->buff = "";
            break ;
        }
        pos = buffer.find("\r\n", i);
        if (pos == std::string::npos){
            pos = buffer.find("\n", i);
            if (pos == std::string::npos && buffer.length() > 0){
                clt->buff = buffer.substr(i, buffer.length() - i);
                return true;
            }
            flag = 0;
        }
        std::string line = buffer.substr(i, pos - i);
        if (!line.find("PASS ", 0)){
            std::string pass = line.substr(5, line.length() - 5);
            if (pass != data->sock->password){
                std::string err_msg = ":irc.leet.ma 464 * :Password incorrect\r\n";
                send(clt->client_fd, err_msg.c_str(), err_msg.length(), 0);
                std::cout << "\nWrong password\n";
                ClearAfterDisconnection(clt->client_fd, data);
                return false;
            }
            std::cout << "\nClient connected.\n";
        }
        if (!line.find("NICK ", 0)){
            std::string new_nick = line.substr(5, line.length() - 5);
            if (HandleDoubleNick(data, new_nick) == false){
                std::string err_msg = ":irc.leet.ma 433 * " + new_nick + " :Nickname is already in use\r\n";
                send(clt->client_fd, err_msg.c_str(), err_msg.length(), 0);
                if(clt->nickname.empty()){
                    ClearAfterDisconnection(clt->client_fd, data);
                    return false;
                }
                std::cout << "\nNickName already exist\n";
            }
            else {
                std::string old_nick = clt->nickname;
                clt->nickname = new_nick;
                std::string nick_change_msg = ":" + old_nick + "!" + clt->username + "@localhost NICK :" + clt->nickname + "\r\n";
    
                for (std::vector<Client>::iterator ch = data->clt.begin(); ch != data->clt.end(); ++ch) {
                    send(ch->client_fd, nick_change_msg.c_str(), nick_change_msg.length(), 0);
                }
            }
        }
        if (!line.find("USER ", 0)){
            std::string temp = line.substr(5, line.length() - 5);
            int n = temp.find(" ", 0);
            clt->username = temp.substr(0, n);
        }
        if (!line.find("JOIN ", 0)){
            std::string chan = line.substr(5, line.length() - 5);
            HandleChannels(chan, data, clt);
        }
        if (!line.find("PRIVMSG ", 0)){
            std::string msg = line.substr(8, line.length() - 8);
            HandlePrivateMsg(msg, data, clt);
        }
        if (!line.find("KICK ", 0)){
            std::string temp = line.substr(5, line.length() - 5);
            KickUserFromChannel(temp, data, clt);
        }
        if (!line.find("MODE ", 0)){
            std::string temp = line.substr(5,line.length() - 5);
            HandleChannelModes(temp, data, clt);
        }
        if (!line.find("INVITE ")){
            std::string temp = line.substr(7, line.length() - 7);
            InviteUserToChannel(temp, data, clt);
        }
        if (!line.find("TOPIC ", 0)){
            std::string temp = line.substr(6, line.length() - 6);
            HandleTopic(temp, clt, data);
        }
        i = pos + 1 + flag;
    }
    return true;
}

bool    RecvNew(epoll_event ev, Server *data)
{
    char    buffer[LARG_NUMBER];
    Client *clt = FindClient(ev.data.fd, data);
    if(!clt){
        std::cout << "Client not found \n";
        return false;
    }
    int rd = recv(ev.data.fd, buffer, (LARG_NUMBER) - 1, 0);
    std::cout << buffer << std::endl;
    if (rd > 0){
        buffer[rd] = '\0';
        std::string msg = clt->buff + (std::string)buffer;
        if (HandleBuffer(msg, ev.data.fd, data) == false){
            ClearAfterDisconnection(clt->client_fd, data);
        }
    } else if (rd == 0) {
        std::cout << "\nClient disconnected.\n";
        ClearAfterDisconnection(clt->client_fd, data);
    } 
    else if (rd < 0) {
        std::cerr << "recv failed\n";
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
