#include "Server.hpp"

Server::Server(){
    sock = new Socket;

}

Server::~Server(){
    if (this->sock->sockfd){
        close(this->sock->sockfd);
    }
    delete sock;
}
