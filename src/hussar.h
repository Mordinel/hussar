/**
*     Copyright (C) 2022 Mason Soroka-Gill
* 
*     This program is free software: you can redistribute it and/or modify
*     it under the terms of the GNU General Public License as published by
*     the Free Software Foundation, either version 3 of the License, or
*     (at your option) any later version.
* 
*     This program is distributed in the hope that it will be useful,
*     but WITHOUT ANY WARRANTY; without even the implied warranty of
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*     GNU General Public License for more details.
* 
*     You should have received a copy of the GNU General Public License
*     along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "libs.h"
#include "request.h"
#include "response.h"
#include "router.h"
#include "config.h"

namespace hussar {
    class Hussar {
    private:
        int sockfd;                     // Server socket
        sockaddr_in client_address;      // client addr struct

        hussar::Config config;
        ThreadPool thread_pool;

        SSL_CTX* ctx;

    public:
        hussar::Router router;

    private:

        /**
         * handles a single client connection
         */
        void handle_connection(int client, char* host, SSL* ssl) {
            // allocate buffer for service string
            char svc[NI_MAXSERV];
            memset(svc, 0, NI_MAXSERV);
        
            // allocate a buffer for recieved data
            char buf[4096];

            if (this->config.verbose) {
                print_lock.lock();
                    std::cout << host <<" connected" << std::endl;
                print_lock.unlock();
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
                        if (this->config.verbose) {
                            print_lock.lock();
                               std::cerr << "There was a connection issue with " << host << std::endl;
                            print_lock.unlock();
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
 
                        this->router.route(req, resp);

                        // we do a little logging
                        if (this->config.verbose) {
                            print_lock.lock();
                            std::cout << req.remote_host <<
                                "\t" << resp.headers["Date"] <<
                                "\t" << strip_terminal_chars(req.method) <<
                                "\t" << resp.code <<
                                "\t" << strip_terminal_chars(req.document_raw) <<
                                "\t" << strip_terminal_chars(req.user_agent) <<
                                "\n";
                            print_lock.unlock();
                        }

                        std::string response = resp.serialize();
                        if (ssl) {
                            SSL_write(ssl, response.c_str(), response.size());
                        } else {
                            send(client, response.c_str(), response.size(), 0);
                        }

                        // if keep alive
                        if (req.keep_alive) {
                            continue;
                        }

                        goto srv_disconnect;
                        break;
                }
            }

        srv_disconnect:
            if (this->config.verbose) {
                print_lock.lock();
                    std::cout << host << " disconnected" << std::endl;
                print_lock.unlock();
            }

            if (ssl) {
                SSL_shutdown(ssl);
                SSL_free(ssl);
            }
            close(client);
            std::free(host);
        }

        void init_socket()
        {
            if (this->config.verbose) {
                std::cout << "Binding socket " << this->config.host << ":" << this->config.port << std::endl;
            }

            this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (this->sockfd < 0) {
                fatal_error("ERROR opening socket");
            }

            int reuseAddrOpt = 1;
            setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseAddrOpt, sizeof(reuseAddrOpt));

            sockaddr_in hint;
            hint.sin_family = AF_INET;
            hint.sin_port = htons(this->config.port);
            inet_pton(AF_INET, this->config.host.c_str(), &hint.sin_addr);

            if (bind(this->sockfd, (sockaddr*)&hint, sizeof(hint)) < 0) {
                fatal_error("ERROR can't bind to ip/port");
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
                fatal_error("ERROR can't create SSL context");
            }

            if (SSL_CTX_use_certificate_file(this->ctx, cert.c_str(), SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                fatal_error("ERROR can't use certificate pem file: " + cert);
            }

            if (SSL_CTX_use_PrivateKey_file(this->ctx, privkey.c_str(), SSL_FILETYPE_PEM) <= 0) {
                ERR_print_errors_fp(stderr);
                fatal_error("can't use privatekey pem file: " + privkey);
            }

        }

    public:

        Hussar(Config& config)
            : config(std::move(config)), thread_pool(this->config.thread_count), ctx(nullptr)
        {
            print_lock.unlock();
            openssl_rand_lock.unlock();
            sessions_lock.unlock();
            this->init_socket();

            // if ssl
            if (this->config.certificate != "" && this->config.private_key != "") {
                if (this->config.verbose) {
                    print_lock.lock();
                        std::cout << "SSL ENABLED" << std::endl;
                    print_lock.unlock();
                }

                this->init_ssl_context(this->config.certificate, this->config.private_key);
            }

            if (this->config.verbose) {
                // print link
                print_lock.lock();
                    std::cout << "Server available at: ";
                    if (this->ctx) { // if ssl
                        std::cout << "https://";
                    } else {
                        std::cout << "http://";
                    }
                    std::cout << this->config.host << ":" << this->config.port << "/\n";
                print_lock.unlock();
            }
        }

        // delete copy constructors
        Hussar(Hussar& h) = delete;
        Hussar(const Hussar& h) = delete;
        Hussar& operator=(Hussar& h) = delete;
        Hussar& operator=(const Hussar& h) = delete;

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
        void serve()
        {
            if (listen(this->sockfd, SOMAXCONN) < 0) {
                fatal_error("ERROR can't listen");
            }
        
            while (true) {
                socklen_t clientSize = sizeof(this->client_address);
                // accept connections
                int client_socket = accept(this->sockfd, (sockaddr*)&this->client_address, &clientSize);

                // get the host address of the tcp client
                char* host = (char*)std::calloc(NI_MAXHOST, sizeof(char));
                inet_ntop(AF_INET, &this->client_address.sin_addr, host, NI_MAXHOST);

                if (client_socket < 0) {
                    if (this->config.verbose) {
                        print_lock.lock();
                            std::cerr << "ERROR problem with client connection" << std::endl;
                        print_lock.unlock();
                    }
                    std::free(host);
                } else if (this->ctx) {
                    SSL* ssl = SSL_new(this->ctx);
                    SSL_set_fd(ssl, client_socket);
                    if (SSL_accept(ssl) > 0) {
                        this->thread_pool.dispatch(&Hussar::handle_connection, this, client_socket, host, ssl);
                    }
                } else {
                    this->thread_pool.dispatch(&Hussar::handle_connection, this, client_socket, host, nullptr);
                }
            }
        }
    };
}

