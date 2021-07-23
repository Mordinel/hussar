#include <unistd.h> // for getopt
#include <iostream>
#include <string>

#include "hussar.h"

void printHelp(char* arg0)
{
    std::cout << "Usage: " << arg0 << " [-hv -p <port> -t <thead count> -d <document root>]\n";
    std::cout << "\t-h\t\tDisplay this help\n";
    std::cout << "\t-v\t\tVerbose console output\n";
    std::cout << "\t-p <PORT>\tPort to listen on\n";
    std::cout << "\t-t <THREAD>\tThreads to use\n";
    std::cout << "\t-d <DIR>\tDocument root directory\n";
}

int main(int argc, char* argv[])
{
    std::string docRoot = "";
    std::string host = "127.0.0.1";
    unsigned short port = 8080;
    unsigned int threads = 0; // 0 defaults to max hardware threads
    bool verbose = false;

    std::stringstream ss;

    int c;
    while ((c = getopt(argc, argv, "hvp:t:d:")) != -1) {
        switch (c) {

            case 'h':
                printHelp(argv[0]);
                return 1;

            case 'v':
                verbose = true;
                break;

            case 'p':
                ss.clear();
                ss << optarg;
                ss >> port;
                if (ss.fail()) {
                    std::cerr << "Error: " << optarg << " is not a valid port number, defaulting to (8080)\n";
                    port = 8080;
                }
                break;

            case 't':
                ss.clear();
                ss << optarg;
                ss >> threads;
                if (ss.fail()) {
                    std::cerr << "Error: " << optarg << " is not a valid value for thread count, defaulting to (0)\n";
                    threads = 0;
                }
                break;
 
            case 'd':
                docRoot = optarg;
                break;
       }
    }

    Hussar server(host, port, docRoot, threads, verbose);
    server.Listen();

    return 0;
}
