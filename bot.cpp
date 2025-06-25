// #include "Server.hpp"


// void    InitBot(Bot *Ibot){
//     Ibot = new Bot;
//     Ibot->sever_ip = "127.0.0.1";
//     Ibot->user = "bot";
//     Ibot->user = "LeetBot";
//     Ibot->port = g_data->sock->port;
//     Ibot->pass = g_data->sock->password;
//     Ibot->channel = "#leetchat";
//     Ibot->bot_addr.sin_family = AF_INET;
//     Ibot->bot_addr.sin_port = htons(Ibot->port);
//     Ibot->bot_addr.sin_addr.s_addr = INADDR_ANY;
//     inet_pton(AF_INET, Ibot->sever_ip.c_str(), &Ibot->bot_addr.sin_addr);
// }


// int main(){
//     Bot *Ibot;

//     Ibot->fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (Ibot->fd < 0){
//         delete Ibot;
//         return 1;
//     }
//     if(connect(Ibot->fd, (sockaddr *)&Ibot->bot_addr, sizeof(Ibot->bot_addr)) < 0){
//         std::cout << "connect failed\n";
//         close(Ibot->fd);
//         delete Ibot;
//         return 1;
//     }
//     std::string msg = "PASS " + Ibot->pass + "\r\n";
//     send(Ibot->fd, msg.c_str(), msg.length(), 0);
//     msg = "NICK " + Ibot->nick + "\r\n";
//     send(Ibot->fd, msg.c_str(), msg.length(), 0);
//     msg = "USER " + Ibot->user + " 0 * :" + Ibot->user + "\r\n";
//     send(Ibot->fd, msg.c_str(), msg.length(), 0);
//     msg = "JOIN " + Ibot->channel + "\r\n";
//     send(Ibot->fd, msg.c_str(), msg.length(), 0);

// }