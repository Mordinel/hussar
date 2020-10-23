#include <iostream>
#include <string>

#include "hussar.h"

int main(int argc, char* argv[])
{
    const std::string host = "127.0.0.1";
    const unsigned int port = 8080;

    Hussar hussar(host, port);

    return 0;
}
