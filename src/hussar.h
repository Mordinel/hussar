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

        SSL_CTX* ctx;

    public:
        Router Router;

    private:

        /**
         * handles a single client connection
         */
        void handleConnection(int client, SSL* ssl) {
            // allocate stack buffers for host and service strings
            char host[NI_MAXHOST];
            char svc[NI_MAXSERV];
        
            // set all chars to null in each buffer
            memset(host, 0, NI_MAXHOST);
            memset(svc, 0, NI_MAXSERV);
            
            inet_ntop(AF_INET, &this->clientAddress.sin_addr, host, NI_MAXHOST);
        
            // allocate a stack buffer for recieved data
            char buf[4096];

            if (this->verbose) {
                PrintLock.lock();
                    std::cout << host <<" connected" << std::endl;
                PrintLock.unlock();
            }

            // read and handle bytes until the connection ends
            while (true) {
                // set the recv buffer to null chars
                memset(buf, 0, 4096);
        
                // read data incoming from the client into the recv buffer
                int status;
                if (ssl) {
                    status = SSL_read(ssl, buf, 4096);
                } else {
                    status = recv(client, buf, 4094, 0);
                }
        
                switch (status) {
                    case -1: // connection error
                        if (this->verbose) {
                            PrintLock.lock();
                               std::cerr << "There was a connection issue with " << host << std::endl;
                            PrintLock.unlock();
                        }
                        goto srv_disconnect; // disconnect
                        break;
                    case 0: // client disconnected
                        goto srv_disconnect; // disconnect
                        break;
                    default:
                        std::string bufStr{buf};
                        Request req{bufStr, host};
                        Response resp{req};
 
                        this->Router.Route(req, resp);

                        std::string response = resp.Serialize();
                        if (ssl) {
                            SSL_write(ssl, response.c_str(), response.size());
                        } else {
                            send(client, response.c_str(), response.size(), 0);
                        }

                        // if keep alive
                        if (req.KeepAlive) {
                            continue;
                        }

                        goto srv_disconnect;
                        break;
                }
            }

        srv_disconnect:
            if (this->verbose) {
                PrintLock.lock();
                    std::cout << host << " disconnected" << std::endl;
                PrintLock.unlock();
            }

            if (ssl) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
            }
            close(client);
        }

        void init_socket()
        {
            if (this->verbose) {
                std::cout << "Binding socket " << this->host << ":" << this->port << std::endl;
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
            inet_pton(AF_INET, this->host.c_str(), &hint.sin_addr);

            if (bind(this->sockfd, (sockaddr*)&hint, sizeof(hint)) < 0) {
                Error("ERROR can't bind to ip/port");
            }
        }

        void init_ssl_context(const std::string& cert, const std::string& privkey)
        {
            SSL_load_error_strings();
            SSL_library_init();
            OpenSSL_add_all_algorithms();

            const SSL_METHOD* method = TLS_server_method();
            this->ctx = SSL_CTX_new(method);
            if (not this->ctx) {
                ERR_print_errors_fp(stderr);
                Error("ERROR can't create SSL context");
            }

            if (SSL_CTX_use_certificate_file(this->ctx, cert.c_str(), SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                Error("ERROR can't use certificate pem file: " + cert);
            }

            if (SSL_CTX_use_PrivateKey_file(this->ctx, privkey.c_str(), SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                Error("can't use privatekey pem file: " + privkey);
            }

        }

    public:

        /**
         * binds a host:port socket
         */
        Hussar(const std::string& host, const unsigned short port, unsigned int threadcount, bool verbose)
            : host(host), port(port), threadpool(threadcount), verbose(verbose), ctx(nullptr)
        {
            PrintLock.unlock();
            this->init_socket();
        }

        /**
         * binds a host:port SSL socket
         */
        Hussar(const std::string& host, const unsigned short port, unsigned int threadcount, const std::string& privkey, const std::string& cert, bool verbose)
            : host(std::move(host)), port(port), threadpool(threadcount), verbose(verbose), ctx(nullptr)
        {
            PrintLock.unlock();
            if (this->verbose) {
                PrintLock.lock();
                    std::cout << "SSL ENABLED" << std::endl;
                PrintLock.unlock();
            }
            this->init_socket();
            this->init_ssl_context(cert, privkey);
        }


        // closes the server socket
        ~Hussar()
        {
            if (this->ctx) {
                SSL_CTX_free(this->ctx);
                ERR_free_strings();
                EVP_cleanup();
            }
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
                    if (this->verbose) {
                        PrintLock.lock();
                            std::cerr << "ERROR problem with client connection" << std::endl;
                        PrintLock.unlock();
                    }
                } else if (this->ctx) {
                    SSL* ssl = SSL_new(this->ctx);
                    SSL_set_fd(ssl, clientSocket);
                    if (SSL_accept(ssl) > 0) {
                        this->threadpool.Dispatch(&Hussar::handleConnection, this, clientSocket, ssl);
                    }
                } else {
                    this->threadpool.Dispatch(&Hussar::handleConnection, this, clientSocket, nullptr);
                }
            }
        }
    };
}

#endif
