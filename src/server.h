#ifndef SERVER_H
#define SERVER_H

// c includes
#include <cstdlib>      // exit
#include <netdb.h>      // getnameinfo
#include <unistd.h>     // close
#include <string.h>     // memset
#include <signal.h>     // sigs
#include <arpa/inet.h>  // htons
#include <sys/types.h>  // compatibility reasons
#include <netinet/in.h>
#include <sys/socket.h> // sockets

// cpp includes
#include <iostream>
#include <fstream>
#include <string>

class Server {
    private:
        int sockfd;                 // server socket
        sockaddr_in clientAddress;  // client addr struct

        unsigned short port;
        std::string host;

        void error(const std::string& message); // fatal errors that require an exit() call
        void handleConnection(int client);      // handles connections
    public:
        Server(const std::string& host, const unsigned short port);
        ~Server();
        void Listen();
};


#endif
