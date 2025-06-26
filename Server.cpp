#include "Server.hpp"

Server::Server(){
    sock = new Socket;

}

Server::~Server(){
    if (this->sock->sockfd){
        close(this->sock->sockfd);
    }
    if (this->epfd){
        close(this->epfd);
    }
    delete sock;
    std::cout << "\ndeleting sock\n";
}
