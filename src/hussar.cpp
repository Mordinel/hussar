#include "hussar.h"

/**
 * binds a host:port socket, then calls this->Listen to listen for incoming connections
 */
Hussar::Hussar(const std::string& host, const unsigned short port, const std::string& docRoot, unsigned int threadcount, bool verbose)
    : host(std::move(host)), port(port), docRoot(docRoot), printLock(this->printMut), threadpool(threadcount), verbose(verbose)
{
    this->printLock.unlock();

    if (this->verbose) {
        std::cout << "Binding socket " << host << ":" << port << "\n";
    }

    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->sockfd < 0) {
        this->error("ERROR opening socket");
    }

    int reuseAddrOpt = 1;
    setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &reuseAddrOpt, sizeof(reuseAddrOpt));

    sockaddr_in hint; 
    hint.sin_family = AF_INET;
    hint.sin_port = htons(this->port);
    inet_pton(AF_INET, host.c_str(), &hint.sin_addr);

    if (bind(this->sockfd, (sockaddr*)&hint, sizeof(hint)) < 0) {
        this->error("ERROR can't bind to ip/port");
    }
}

// closes the server socket
Hussar::~Hussar()
{
    close(this->sockfd);
}

/**
 * listen for incoming connections and spawn threads for each connection
 */
void Hussar::Listen()
{
    if (listen(this->sockfd, SOMAXCONN) < 0) {
        this->error("ERROR can't listen");
    }

    while (true) {
        socklen_t clientSize = sizeof(this->clientAddress);

        // accept connections
        int clientSocket = accept(this->sockfd, (sockaddr*)&this->clientAddress, &clientSize);

        if (clientSocket < 0) {
            this->printLock.lock();
                std::cerr << "ERROR problem with client connection" << std::endl;
            this->printLock.unlock();
        } else {
            this->threadpool.Dispatch(&Hussar::handleConnection, this, clientSocket, SOCKET_MAXTIME);
        }
    }
}

/**
 * handles a single client connection
 */
void Hussar::handleConnection(int client, int timeout) {
    auto start = std::chrono::steady_clock::now();
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
                this->printLock.lock();
                    std::cerr << "There was a connection issue with " << host << std::endl;
                this->printLock.unlock();
                goto srv_disconnect; // disconnect
                break;
            case 0: // client disconnected
                this->printLock.lock();
                    std::cout << host << " disconnected" << std::endl;
                this->printLock.unlock();
                goto srv_disconnect; // disconnect
                break;
            default:

                std::string bufStr(buf);
                Request r(bufStr);

                std::string* response = this->handleRequest(r, client, host);
                send(client, response->c_str(), response->size(), 0);
                delete response;

                goto srv_disconnect;
                break;
        }
    }

srv_disconnect:
    close(client);
}

std::string* Hussar::handleRequest(Request& req, int client, char* host)
{
    if (req.isRequestGood) {
        std::vector<std::string> docInfo;
        this->serveDoc(req.Document, this->docRoot, docInfo);

        std::string body = std::move(docInfo[0]);
        std::string mime = std::move(docInfo[1]);
        std::string http = std::move(docInfo[2]);


        std::string status;
        // add more statuses later
        if (http == "200") {
            status = "OK";
        } else if (http == "404") {
            status = "NOT FOUND";
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
            this->printLock.lock();
            if (req.UserAgent.size()) {
                std::cout << date << "\t" << host << "\t" << req.Method << "\t" << http << "\t" << req.DocumentOriginal << '?' << req.GetParameters << "\t" << req.UserAgent << "\n";
            } else {
                std::cout << date << "\t" << host << "\t" << req.Method << "\t" << http << "\t" << req.DocumentOriginal << '?' << req.GetParameters << "\n";
            }
            this->printLock.unlock();
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

        std::string* response = new std::string(responseStream.str());
        return response;
    }

    // MAKE THIS RETURN A 500 ERROR
    std::string* response = new std::string("BAD REQUEST");
    return response;
}

/**
 * attempts to read the document from the filesystem, 
 */
void Hussar::serveDoc(std::string& document, const std::string& docRoot, std::vector<std::string>& docInfo)
{
    std::regex slashes("/+");
    std::regex dots("[.][.]+");
    document = std::regex_replace(document, slashes, "/"); // collapse slashes into a single slash
    document = std::regex_replace(document, dots, ""); // collapse 2 or more dots into nothing

    // if a directory traversal is attempted, make the document target index.html
    if (document.find("../") != std::string::npos) {
        document = "/index.html";
    } else if (document == "/") { // if directory is /, then make the document target index.html
        document = "/index.html";
    }

    // get the absolute path of the requested document
    std::filesystem::path p(std::filesystem::current_path());
    p /= docRoot;
    p += document;
    p = std::filesystem::weakly_canonical(p);

    // docInfo[0] = file data
    // docInfo[1] = mime type
    // docInfo[2] = http status
    if (std::filesystem::exists(p)) {
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
std::string* Hussar::getMime(std::string& document) {
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

// for fatal errors that should kill the program.
void Hussar::error(const std::string& message)
{
    this->printLock.lock();
        std::cerr << message << std::endl;
    this->printLock.unlock();
    std::exit(1);
}

