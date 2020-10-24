#ifndef SERVER_H
#define SERVER_H

// c includes
#include <cstdlib>      // exit
#include <netdb.h>      // getnameinfo
#include <unistd.h>     // close
#include <string.h>     // memset
#include <signal.h>     // sigs
#include <pthread.h>    // spinlocking
#include <arpa/inet.h>  // htons
#include <sys/types.h>  // compatibility reasons
#include <netinet/in.h>
#include <sys/socket.h> // sockets

// cpp includes
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>

#include "request.h"

#define SERVER_NAME "HussarHTTP"
#define SOCKET_MAXTIME 30

class Server {
    private:
        int sockfd;                     // server socket
        sockaddr_in clientAddress;      // client addr struct
        pthread_spinlock_t spinlock;    // for file threaded file access

        unsigned short port;
        std::string host;
        std::string execDir;
        std::string docRoot;

        void error(const std::string& message);         // fatal errors that require an exit() call
        void handleConnection(int client, int timeout); // handles connections
        std::string* handleRequest(Request& req, int client);    // handles requests
        void serveDoc(std::string& document, const std::string& docRoot, std::vector<std::string>& docInfo);
    public:
        Server(const std::string& host, const unsigned short port, const std::string& docRoot);
        ~Server();
        void Listen();
};


#endif
