#ifndef HUSSAR_H
#define HUSSAR_H

#include "pch.h"
#include "request.h"
#include "response.h"
#include "router.h"

namespace hussar {
    class Hussar {
    private:
        int sockfd;                     // Server socket
        sockaddr_in clientAddress;      // client addr struct

        std::string host;
        unsigned short port;
        ThreadPool threadpool;
        bool verbose;

    public:
        Router Router;

    private:
        /**
         * handles a single client connection
         */
        void handleConnection(int client, int timeout) {
        #ifdef DEBUG
            PrintLock.lock();
                std::cout << "Connection opened.\n";
            PrintLock.unlock();
        #endif

            // allocate stack buffers for host and service strings
            char host[NI_MAXHOST];
            char svc[NI_MAXSERV];
        
            // set all chars to null in each buffer
            memset(host, 0, NI_MAXHOST);
            memset(svc, 0, NI_MAXSERV);
            
            inet_ntop(AF_INET, &this->clientAddress.sin_addr, host, NI_MAXHOST);
        
            // allocate a stack buffer for recieved data
            char buf[4096];

            // read and handle bytes until the connection ends
            while (true) {
                // set the recv buffer to null chars
                memset(buf, 0, 4096);
        
                // read data incoming from the client into the recv buffer
                int bytesRecv = recv(client, buf, 4096, 0);
        
                switch (bytesRecv) {
                    case -1: // connection error
                        PrintLock.lock();
                           std::cerr << "There was a connection issue with " << host << std::endl;
                        PrintLock.unlock();
                        goto srv_disconnect; // disconnect
                        break;
                    case 0: // client disconnected
                        PrintLock.lock();
                            std::cout << host << " disconnected" << std::endl;
                        PrintLock.unlock();
                        goto srv_disconnect; // disconnect
                        break;
                    default:
                        std::string bufStr{buf};
                        Request req{bufStr, host};
                        Response resp{req};
 
                        if (req.Method == "GET") {
                            this->Router.GET(req, resp);
                        } else if (req.Method == "HEAD") {
                            this->Router.HEAD(req, resp);
                        } else if (req.Method == "POST") {
                            this->Router.POST(req, resp);
                        } else {
                            this->Router.DEFAULT(req, resp);
                        }

                        std::string response = resp.Serialize();
                        send(client, response.c_str(), response.size(), 0);

                        // if keep alive
                        if (req.KeepAlive) {
                            continue;
                        }

                        goto srv_disconnect;
                        break;
                }
            }

        srv_disconnect:
        #ifdef DEBUG
            PrintLock.lock();
                std::cout << "Connection closed.\n";
            PrintLock.unlock();
        #endif

            close(client);
        }

    public:

        /**
         * binds a host:port socket
         */
        Hussar(const std::string& host, const unsigned short port, unsigned int threadcount, bool verbose)
            : host(std::move(host)), port(port), threadpool(threadcount), verbose(verbose)
        {
            PrintLock.unlock();
        
            if (this->verbose) {
                std::cout << "Binding socket " << host << ":" << port << "\n";
            }
        
            this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (this->sockfd < 0) {
                Error("ERROR opening socket");
            }
        
            int reuseAddrOpt = 1;
            setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseAddrOpt, sizeof(reuseAddrOpt));
        
            sockaddr_in hint; 
            hint.sin_family = AF_INET;
            hint.sin_port = htons(this->port);
            inet_pton(AF_INET, host.c_str(), &hint.sin_addr);
        
            if (bind(this->sockfd, (sockaddr*)&hint, sizeof(hint)) < 0) {
                Error("ERROR can't bind to ip/port");
            }
        }

        // closes the server socket
        ~Hussar()
        {
            close(this->sockfd);
        }

        /**
         * listen for incoming connections and spawn threads for each connection
         */
        void Listen()
        {
            if (listen(this->sockfd, SOMAXCONN) < 0) {
                Error("ERROR can't listen");
            }
        
            while (true) {
                socklen_t clientSize = sizeof(this->clientAddress);
        
                // accept connections
                int clientSocket = accept(this->sockfd, (sockaddr*)&this->clientAddress, &clientSize);
        
                if (clientSocket < 0) {
                    PrintLock.lock();
                        std::cerr << "ERROR problem with client connection" << std::endl;
                    PrintLock.unlock();
                } else {
                    this->threadpool.Dispatch(&Hussar::handleConnection, this, clientSocket, SOCKET_MAXTIME);
                }
            }
        }
    };
}

#endif
