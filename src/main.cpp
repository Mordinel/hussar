#include <iostream>
#include <string>

#include "server.h"

int main(int argc, char* argv[])
{
    const std::string docRoot= "docroot";
    const std::string host = "127.0.0.1";
    const unsigned short port = 8080;

    Server server(host, port, docRoot);

    return 0;
}
