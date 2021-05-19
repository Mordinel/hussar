#include <iostream>
#include <string>

#include "hussar.h"

int main(int argc, char* argv[])
{
    const std::string docRoot= "docroot";
    const std::string host = "127.0.0.1";
    const unsigned short port = 8080;
    const unsigned int threads = 0; // 0 defaults to max hardware threads

    Hussar server(host, port, docRoot, threads);
    server.Listen();

    return 0;
}
