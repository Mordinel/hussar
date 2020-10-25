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
#include <unordered_map>    // keeps threads in scope
#include <filesystem>       // file reading
#include <iostream>
#include <fstream>          // file streams
#include <string>           // strings and stringstreams
#include <thread>           // threading
#include <mutex>            // thread resource locking

#include "request.h"

#define SERVER_NAME "HussarHTTP"
#define SOCKET_MAXTIME 30

class Server {
    private:
        int sockfd;                     // server socket
        sockaddr_in clientAddress;      // client addr struct

        std::string host;
        unsigned short port;
        std::string docRoot;
        std::mutex fileMutex;
        std::unordered_map<std::thread::id, std::jthread> threads;

        void error(const std::string& message);         // fatal errors that require an exit() call
        void handleConnection(int client, int timeout); // handles connections
        std::string* handleRequest(Request& req, int client);    // handles requests
        void serveDoc(std::string& document, const std::string& docRoot, std::vector<std::string>& docInfo);
        void eraseThread(std::thread::id tid);
    public:
        Server(const std::string& host, const unsigned short port, const std::string& docRoot);
        ~Server();
        void Listen();
};


#endif
