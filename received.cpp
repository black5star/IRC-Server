#include "Server.hpp"

Client *FindClient(int fd, Server * data) {
    
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it) {
        if (it->client_fd == fd) {
            return &(*it);
        }
    }
    return NULL;
}

bool is_auth(Client *cli) {    
    if (cli->auth == false)
    {
        std::string res =  ":irc.leet.ma 451 * :You have not registred \r\n"; 
        send(cli->client_fd, res.c_str(), res.length(), 0);
    }
    return cli->auth;   
}

bool    ClearAfterDisconnection(int client_fd, Server *data) {
    if (epoll_ctl(data->epfd, EPOLL_CTL_DEL, client_fd, NULL) < 0) {
        return false;
    }
    for (std::map<std::string, Channel>::iterator it = data->channels.begin(); it != data->channels.end();) {
        Channel &chan = it->second;
        chan.members.erase(client_fd);
        chan.operators.erase(client_fd);
        chan.invited.erase(client_fd);
        if (chan.members.empty()){
            std::map<std::string, Channel>::iterator temp = it;
            ++it;
            data->channels.erase(temp);
        }
        else
            ++it;
    }
    for (std::vector<Client>::iterator it = data->clt.begin(); it != data->clt.end(); ++it) {
        if (it->client_fd == client_fd) {
            data->clt.erase(it);
            break;
        }
    }
    return true;
}

bool    HandleBuffer(std::string buffer, int fd, Server *data) {
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
        std::cout << line << std::endl;
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
            clt->auth = true;
        }else if (!line.find("NICK ", 0) && is_auth(clt)){
            std::string new_nick = line.substr(5, line.length() - 5);
            if (HandleDoubleNick(data, new_nick) == false){
                std::string err_msg = ":irc.leet.ma 433 * " + new_nick + " :Nickname is already in use\r\n";
                send(clt->client_fd, err_msg.c_str(), err_msg.length(), 0);
                close(clt->client_fd);
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
        } else if (!line.find("USER ", 0) && is_auth(clt)){
            std::string temp = line.substr(5, line.length() - 5);
            int n = temp.find(" ", 0);
            clt->username = temp.substr(0, n);
        } else if (!line.find("JOIN ", 0) && is_auth(clt)){
            std::string chan = line.substr(5, line.length() - 5);
            HandleChannels(chan, data, clt);
        } else if (!line.find("PRIVMSG ", 0) && is_auth(clt)){
            std::string msg = line.substr(8, line.length() - 8);
            HandlePrivateMsg(msg, data, clt);
        }else if (!line.find("KICK ", 0) && is_auth(clt)){
            std::string temp = line.substr(5, line.length() - 5);
            KickUserFromChannel(temp, data, clt);
        } else if (!line.find("MODE ", 0) && is_auth(clt)){
            std::string temp = line.substr(5,line.length() - 5);
            HandleChannelModes(temp, data, clt);
        } else if (!line.find("INVITE ") && is_auth(clt)){
            std::string temp = line.substr(7, line.length() - 7);
            InviteUserToChannel(temp, data, clt);
        } else if (!line.find("TOPIC ", 0) && is_auth(clt) ){
            std::string temp = line.substr(6, line.length() - 6);
            HandleTopic(temp, clt, data);
        } else{
            std::string msg = "Unrecognised command !!\n";
            send(clt->client_fd, msg.c_str(), msg.length(), 0);
        }
        i = pos + 1 + flag;
    }
    return true;
}

bool    RecvNew(epoll_event ev, Server *data) {
    char    buffer[LARG_NUMBER];
    Client *clt = FindClient(ev.data.fd, data);
    if(!clt){
        std::cout << "Client not found \n";
        return false;
    }
    int rd = recv(ev.data.fd, buffer, (LARG_NUMBER) - 1, 0);
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
