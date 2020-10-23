#ifndef SERVER_H
#define SERVER_H

// c includes
#include <cstdlib>      // exit
#include <netdb.h>      // getnameinfo
#include <string.h>     // memset
#include <arpa/inet.h>  // htons
#include <sys/types.h>  // compatibility reasons
#include <sys/socket.h> // sockets

// cpp includes
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <queue>
#include <set>

#include "socket.h"

class Server {
    private:
        int sockfd;
        std::set<int> clientSockets;
        std::queue<shared_ptr<std::thread>> threads;

        void error(const std::string& message);
        void conn(int client, sockaddr_in address);
    public:
        Server(const std::string& host, const short port);
        ~Server();

        void Listen();
};

#endif
