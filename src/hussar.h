#ifndef HUSSAR_H
#define HUSSAR_H

// c includes
#include <sys/types.h>  // compatibility reasons
#include <sys/socket.h> sockets

// cpp includes
#include <iostream>
#include <string>

#include "socket.h"

class Hussar {
    private:
        int sockfd;
        int sockopt;

        void error(const std::string& message);
    public:
        Hussar(const std::string& host, const unsigned int port);
};

#endif
