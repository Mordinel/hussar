#include <unistd.h> // for getopt
#include <iostream>
#include <string>

#include "hussar.h"

std::string DOCROOT = "";
bool VERBOSE = false;

void printHelp(char* arg0)
{
    std::cout << "Usage: " << arg0 << " [-hv -i <ipv4> -p <port> -t <thread count> -d <document root> -k <ssl private key> -c <ssl certificate>]\n";
    std::cout << "\t-h\t\tDisplay this help\n";
    std::cout << "\t-v\t\tVerbose console output\n";
    std::cout << "\t-i <IPV4>\tIpv4 to bind to\n";
    std::cout << "\t-p <PORT>\tPort to listen on\n";
    std::cout << "\t-t <THREAD>\tThreads to use\n";
    std::cout << "\t-d <DIR>\tDocument root directory\n";
    std::cout << "\t-k <key.pem>\tSSL Private key\n";
    std::cout << "\t-c <cert.pem>\tSSL Certificate\n";
}

void web_server(hus::Request& req, hus::Response& resp)
{
    std::filesystem::path p;
    std::regex slashes("/+");
    std::regex dots("[.][.]+");
    std::string document = req.Document;

    if (req.isGood) {
        document = std::regex_replace(document, slashes, "/"); // collapse slashes into a single slash
        document = std::regex_replace(document, dots, ""); // collapse 2 or more dots into nothing

        // if a directory traversal is attempted, make the document target index.html
        if (document.find("../") != std::string::npos) {
            document = "/index.html";
        } else if (document == "/") { // if directory is /, then make the document target index.html
            document = "/index.html";
        } else if (document == "") { // if directory is nothing somehow
            goto nonexistent_file;
        } else if (document.size() > 255) {
                resp.code = "414";
                resp.status = "URI TOO LONG";
                resp.body = "<h1>414: Uri Too Long!</h1>";
                return;
        }

        // get the absolute path of the requested document
        p = std::filesystem::current_path();
        p /= DOCROOT;
        p += document;
        p = std::filesystem::weakly_canonical(p);

        if (std::filesystem::exists(p)) {
            if (std::filesystem::is_regular_file(p)) {
                // file exists, load it
                std::ifstream file(p);
                resp.code = "200";
                resp.Headers["Content-Type"] = hus::GetMime(document);
                resp.body = std::string(
                            (std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
            } else {
                goto nonexistent_file;
            }
        } else {
        nonexistent_file:
            // try a custom 404 page
            p = std::filesystem::current_path();
            p /= DOCROOT;
            p += "/404.html";
            p = std::filesystem::weakly_canonical(p);

            // if it exists, use it for the 404 page else use the default page
            if (std::filesystem::exists(p)) {
                std::ifstream file(p);
                resp.code = "404";
                resp.body = std::string(
                            (std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
            } else {
                resp.code = "404";
                resp.body = "<h1>404: File Not found!</h1>";
            }
        }

    } else {
        resp.code= "400";
        resp.body = "<h1>400: Bad Request!</h1>";
    }

    // prints the GET and POST request parameters in verbose mode
    if (VERBOSE) {
        hus::PrintLock.lock();
        for (auto& p : req.GET) {
            std::cout << "GET[" << hus::StripString(p.first) << "] = " << hus::StripString(p.second) << "\n";
        }
        for (auto& p : req.POST) {
            std::cout << "POST[" << hus::StripString(p.first) << "] = " << hus::StripString(p.second) << "\n";
        }
        hus::PrintLock.unlock();
    }

    // logging
    hus::PrintLock.lock();
    if (req.UserAgent.size()) {
        std::cout << req.RemoteHost << "\t" << resp.Headers["Date"] << "\t" << hus::StripString(req.Method) << "\t" << resp.code << "\t" << hus::StripString(req.DocumentOriginal) << "\t" << hus::StripString(req.UserAgent) << "\n";
    } else {
        std::cout << req.RemoteHost << "\t" << resp.Headers["Date"] << "\t" << hus::StripString(req.Method) << "\t" << resp.code << "\t" << hus::StripString(req.DocumentOriginal) << "\n";
    }
    hus::PrintLock.unlock();
}

int main(int argc, char* argv[])
{
    std::string host = "127.0.0.1";
    unsigned short port = 8080;
    unsigned int threads = 0; // 0 defaults to max hardware threads
    std::string privkey = "";
    std::string certificate = "";

    std::stringstream ss;

    int c;
    while ((c = getopt(argc, argv, "hvi:p:t:d:k:c:")) != -1) {
        switch (c) {

            case 'h':
                printHelp(argv[0]);
                return 1;

            case 'v':
                VERBOSE = true;
                break;

            case 'i':
                host = optarg;
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
 
            case 'k':
                privkey = optarg;
                break;

            case 'c':
                certificate = optarg;
                break;

            case 'd':
                DOCROOT = optarg;
                break;
       }
    }

    hus::Hussar* server;

    if (
        privkey != "" && certificate != "" &&
        std::filesystem::exists(privkey) && std::filesystem::exists(certificate) &&
        std::filesystem::is_regular_file(privkey) && std::filesystem::is_regular_file(certificate)
    ) {
        server = new hus::Hussar(host, port, threads, privkey, certificate, VERBOSE);
    } else {
        server = new hus::Hussar(host, port, threads, VERBOSE);
    }

    server->Router.DEFAULT(&web_server);
    server->Listen();

    delete server;

    return 0;
}
