#include "hussar.h"

Server::Server(const std::string& host, const short port)
    : host(std::move(host)), port(port)
{
    this->sockfd = sock(AF_INET, SOCK_STREAM, 0);
    if (this->sockfd < 0) {
        this->error("ERROR opening socket");
    }
    
    if (setsockopt(this->sockfs, SOL_SOCKET, SO_REUSEADDR, 1) < 0) {
        this->error("ERROR setting socket option");
    }
    
    sockaddr_in hint; 
    hint.sin_famiy = AF_INET;
    hint.sin_port = htons(this->port);
    inet_pton(AF_INET, host.c_str(), &hint.sin_addr);

    if (bind(this->sockfd, static_cast<sockaddr*>(&hint), sizeof(hint)) < 0) {
        this->error("ERROR can't bind to ip/port");
    }
}

Server::~Server()
{
    for (int socket : clientSockets) {
        close(socket);
    }
}

void Server::Listen()
{
    if (listen(this->sockfd, SOMAXCONN) < 0) {
        this->error("ERROR can't listen");
    }


    while (true) {
        sockaddr_in client;
        socklen_t clientSize = sizeof(client);

        int clientSocket = accept(this->sockfs, static_cast<sockaddr*>(&client), &clientSize);

        if (clientSocket < 0) {
            std::cerr << "ERROR problem with client connection" << std::endl;
        } else {
            // set client timeout
            // MAKE THREADS REMOVE THEMSELVES FROM THREAD DATA STRUCTURE
            threads.push(std::make_shared<std::thread>(this->conn, clientSocket, client));
        }
    }
}

void Server::conn(int client, sockaddr_in address) {
    this->clientSockets.insert(client);

    char host[NI_MAXHOST];
    char svc[NI_MAXSERV];

    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);
    
    int result = getnameinfo(static_cast<sockaddr*>(&address), sizeof(address), host, NI_MAXHOST, svc, NI_MAXSERV, 0)

    if (result) {
        std::cout << host << " connected on " << svc << std::endl;
    } else {
        inet_ntop(AF_INET, &address.sin_addr, host, NI_MAXHOST);
        std::cout << host << " connected on " << ntohs(address.sin_port) << std::endl;
    }

    char buf[4096];

    for (;;) {
        memset(buf, 0, 4096);

        int bytesRecv = recv(client, buf, 4096, 0);
        switch (bytesRecv) {
            case -1:
                std::cerr << "There was a connection issue" << std::endl;
                goto srv_disconnect;
                break;
            case 0:
                std::cout << "The client disconnected" << std::endl;
                goto srv_disconnect;
                break;
            default:
                std::cout << "Recieved: " << string(buf, 0, bytesRecv) << std::endl;
                send(client, buf, bytesRecv + 1, 0);
                break;
        }
    }
srv_disconnect:
    close(client);
    this->clientSockets.erase(client);
}

void Server::error(const std::string& message)
{
    std::cerr << message << std::endl;
    std::exit(1);
}
