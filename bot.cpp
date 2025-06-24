#include "Server.hpp"


void    InitBot(Bot *Ibot){
    Ibot = new Bot;
    Ibot->sever_ip = "127.0.0.1";
    Ibot->user = "bot";
    Ibot->user = "LeetBot";
    Ibot->port = g_data->sock->port;
    Ibot->pass = g_data->sock->password;
    Ibot->channel = "#leetchat";
    Ibot->bot_addr.sin_family = AF_INET;
    Ibot->bot_addr.sin_port = htons(Ibot->port);
    Ibot->bot_addr.sin_addr.s_addr = INADDR_ANY;
}


int main(){
    Bot *Ibot;

    Ibot->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (Ibot->fd < 0){
        delete Ibot;
        return 1;
    }

}