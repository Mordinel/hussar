#include "hussar.h"

Hussar::Hussar(const std::string& host, const unsigned int port)
    : host(std::move(host)), port(port)
{


    this->sockfd = sock(AF_INET, SOCK_STREAM, 0);
    if (this->sockfd < 0) {
        this->error("ERROR opening socket");
    }
    
    this->sockopt = setsockopt(this->sockfs, SOL_SOCKET, SO_REUSEADDR, 1);
    if (this->sockopt < 0) {
        this->error("ERROR setting socket option");
    }
    
    
}

void Hussar::error(const std::string& message)
{
    std::cerr << message << std::endl;
    std::exit(0);
}
