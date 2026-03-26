#include "Server.hpp"

bool check(int var, std::string message) {
    if(var < 0){
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}

void CreateSocket(Server *data, const char *s1, std::string s2) {
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
    new_clt.auth = false;
    data->clt.push_back(new_clt);
    return true;
}