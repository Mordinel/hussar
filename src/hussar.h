#ifndef HUSSAR_H
#define HUSSAR_H

#include "pch.h"
#include "request.h"

#define SERVER_NAME "hussar"
#define SOCKET_MAXTIME 30
namespace hussar {
    class Hussar {
    private:

        int sockfd;                     // Server socket
        sockaddr_in clientAddress;      // client addr struct
    
        std::string host;
        unsigned short port;
        std::string docRoot;
        std::unordered_map<std::string, std::string> mimes = {
            {"woff", "font/woff"},
            {"woff2", "font/woff2"},
            {"js", "application/javascript"},
            {"ttf", "application/octet-stream"},
            {"otf", "application/octet-stream"},
            {"exe", "application/octet-stream"},
            {"pdf", "application/pdf"},
            {"zip", "application/zip"},
            {"ico", "image/x-icon"},
            {"gif", "image/gif"},
            {"png", "image/png"},
            {"jpg", "image/jpeg"},
            {"css", "text/css"},
            {"html", "text/html"},
            {"txt", "text/plain"}
        };
        ThreadPool threadpool;
        bool verbose;
    
  
        /**
         * handles a single client connection
         */
        void handleConnection(int client, int timeout) {
            // allocate stack buffers for host and service strings
            char host[NI_MAXHOST];
            char svc[NI_MAXSERV];
        
            // set all chars to null in each buffer
            memset(host, 0, NI_MAXHOST);
            memset(svc, 0, NI_MAXSERV);
            
            inet_ntop(AF_INET, &this->clientAddress.sin_addr, host, NI_MAXHOST);
        
            // allocate a stack buffer for recieved data
            char buf[4096];
        
            // read and handle bytes until the connection ends
            while (true) {
                // set the recv buffer to null chars
                memset(buf, 0, 4096);
        
                // read data incoming from the client into the recv buffer
                int bytesRecv = recv(client, buf, 4096, 0);
        
                switch (bytesRecv) {
                    case -1: // connection error
                        PrintLock.lock();
                            std::cerr << "There was a connection issue with " << host << std::endl;
                        PrintLock.unlock();
                        goto srv_disconnect; // disconnect
                        break;
                    case 0: // client disconnected
                        PrintLock.lock();
                            std::cout << host << " disconnected" << std::endl;
                        PrintLock.unlock();
                        goto srv_disconnect; // disconnect
                        break;
                    default:
        
                        std::string bufStr(buf);
                        Request r(bufStr);
        
                        std::string response = this->handleRequest(r, client, host);
                        send(client, response.c_str(), response.size(), 0);
        
                        goto srv_disconnect;
                        break;
                }
            }
        
        srv_disconnect:
            close(client);
        }

        /**
         * performs request parsing and response
         */
        std::string handleRequest(Request& req, int client, char* host)
        {
            std::vector<std::string> docInfo;
            std::string body;
            std::string mime;
            std::string http;
            std::string status;
        
            if (req.isRequestGood) {
                this->serveDoc(req.Document, this->docRoot, docInfo);
        
                body = docInfo[0];
                mime = docInfo[1];
                http = docInfo[2];
        
                // add more statuses later
                if (http == "200") {
                    status = "OK";
                } else if (http == "404") {
                    status = "NOT FOUND";
                    mime = "text/html";
                }
            } else {
                body = "<h1>400: Bad Request!</h1>";
                mime = "text/html";
                http = "400";
                status = "BAD REQUEST";
            }
        
            // get local time
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto localTime = *std::localtime(&time);
            
            // format date for http output
            std::stringstream dateStream;
            dateStream << std::put_time(&localTime, "%a, %d %b %Y %H:%M:%S");
            std::string date = dateStream.str();
            
            if (this->verbose) {
                PrintLock.lock();
                if (req.UserAgent.size()) {
                    std::cout << date << "\t" << host << "\t" << StripString(req.Method) << "\t" << http << "\t" << StripString(req.DocumentOriginal) << "\t" << StripString(req.UserAgent) << "\n";
                } else {
                    std::cout << date << "\t" << host << "\t" << StripString(req.Method) << "\t" << http << "\t" << StripString(req.DocumentOriginal) << "\n";
                }
                PrintLock.unlock();
            }
            
            std::string connection = "Closed";
            
            // build response payload to return to client
            std::stringstream responseStream;
            responseStream << "HTTP/1.1 " << http << " " << status << "\n";
            responseStream << "Date: " << date << "\n";
            responseStream << "Server: " << SERVER_NAME << "\n";
            responseStream << "Content-Length: " << body.size() << "\n";
            responseStream << "Content-Type: " << mime << "\n";
            responseStream << "Connection: " << connection << "\n";
            responseStream << "\n";
            responseStream << body;
            
            return responseStream.str();
        }

        /**
         * attempts to read the document from the filesystem, 
         */
        void serveDoc(std::string& document, const std::string& docRoot, std::vector<std::string>& docInfo)
        {
            std::filesystem::path p;
            std::regex slashes("/+");
            std::regex dots("[.][.]+");
            document = std::regex_replace(document, slashes, "/"); // collapse slashes into a single slash
            document = std::regex_replace(document, dots, ""); // collapse 2 or more dots into nothing
        
            // if a directory traversal is attempted, make the document target index.html
            if (document.find("../") != std::string::npos) {
                document = "/index.html";
            } else if (document == "/") { // if directory is /, then make the document target index.html
                document = "/index.html";
            } else if (document == "") { // if directory is nothing somehow
                goto nonexistent_file;
            }
        
            // get the absolute path of the requested document
            p = std::filesystem::current_path();
            p /= docRoot;
            p += document;
            p = std::filesystem::weakly_canonical(p);
        
            // docInfo[0] = file data
            // docInfo[1] = mime type
            // docInfo[2] = http status
            if (std::filesystem::exists(p)) {
                if (std::filesystem::is_regular_file(p)) {
                    // file exists, load it into docInfo
                    std::ifstream file(p);
                    docInfo.push_back(
                            std::string(
                                (std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>()));
        
                    std::string* mime = this->getMime(document);
        
                    docInfo.push_back(*mime);
                    docInfo.push_back("200");
                } else {
                    goto nonexistent_file;
                }
            } else {
            nonexistent_file:
                // try a custom 404 page
                p = std::filesystem::current_path();
                p /= docRoot;
                p += "/404.html";
                p = std::filesystem::weakly_canonical(p);
        
                // if it exists, use it for the 404 page else use the default page
                if (std::filesystem::exists(p)) {
                    std::ifstream file(p);
                    docInfo.push_back(
                            std::string(
                                (std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>()));
                    docInfo.push_back("text/html");
                    docInfo.push_back("404");
                } else {
                    docInfo.push_back("<h1>404: File Not found!</h1>");
                    docInfo.push_back("text/html");
                    docInfo.push_back("404");
                }
            }
        }
 
        /**
         * returns a string containing the mime time of the extension of the document string.
         */
        std::string* getMime(std::string& document) {
            std::size_t l = document.find_last_of('.');
        
            // if no char found
            if (l == std::string::npos) {
                return &this->mimes.begin()->second; // default to 1st item (text file)
            }
        
            // create a string view of the extension
            std::string_view extension(document.begin() + l + 1, document.end());
        
            // find extension in this->mimes
            auto mimeIter = this->mimes.find(std::string(extension));
        
            if (mimeIter == this->mimes.end()) {
                return &this->mimes.begin()->second;
            }
        
            return &mimeIter->second;
        }

    public:

        /**
         * binds a host:port socket
         */
        Hussar(const std::string& host, const unsigned short port, const std::string& docRoot, unsigned int threadcount, bool verbose)
            : host(std::move(host)), port(port), docRoot(docRoot), threadpool(threadcount), verbose(verbose)
        {
            PrintLock.unlock();
        
            if (this->verbose) {
                std::cout << "Binding socket " << host << ":" << port << "\n";
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
            inet_pton(AF_INET, host.c_str(), &hint.sin_addr);
        
            if (bind(this->sockfd, (sockaddr*)&hint, sizeof(hint)) < 0) {
                Error("ERROR can't bind to ip/port");
            }
        }

        // closes the server socket
        ~Hussar()
        {
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
                    PrintLock.lock();
                        std::cerr << "ERROR problem with client connection" << std::endl;
                    PrintLock.unlock();
                } else {
                    this->threadpool.Dispatch(&Hussar::handleConnection, this, clientSocket, SOCKET_MAXTIME);
                }
            }
        }
    };
}

#endif
