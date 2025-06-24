#include "Server.hpp"

Server::Server(){
    sock = new Socket;

}

Server::~Server(){
    if (this->sock->sockfd){
        close(this->sock->sockfd);
        std::cout << "\nsockfd closed\n";
    }
    if (this->epfd){
        close(this->epfd);
        std::cout << "\nepfd closed\n";
    }
    delete sock;
    std::cout << "\ndeleting sock\n";
}
