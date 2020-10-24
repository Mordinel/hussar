#include "server.h"

/**
 * binds a host:port socket, then calls this->Listen to listen for incoming connections
 */
Server::Server(const std::string& host, const unsigned short port, const std::string& docRoot)
    : host(std::move(host)), port(port), docRoot(docRoot)
{
    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->sockfd < 0) {
        this->error("ERROR opening socket");
    }
    
    sockaddr_in hint; 
    hint.sin_family = AF_INET;
    hint.sin_port = htons(this->port);
    inet_pton(AF_INET, host.c_str(), &hint.sin_addr);

    if (bind(this->sockfd, (sockaddr*)&hint, sizeof(hint)) < 0) {
        this->error("ERROR can't bind to ip/port");
    }

    pthread_spin_init(&this->spinlock, 0);

    this->Listen();
}

// closes the server socket
Server::~Server()
{
    close(this->sockfd);
}

/**
 * listen for incoming connections and spawn threads for each connection
 */
void Server::Listen()
{
    if (listen(this->sockfd, SOMAXCONN) < 0) {
        this->error("ERROR can't listen");
    }

    while (true) {
        socklen_t clientSize = sizeof(this->clientAddress);

        // accept connections
        int clientSocket = accept(this->sockfd, (sockaddr*)&this->clientAddress, &clientSize);

        if (fork() != 0) {
            close(clientSocket);
            continue;
        }

        if (clientSocket < 0) {
            std::cerr << "ERROR problem with client connection" << std::endl;
        } else {
            this->handleConnection(clientSocket, SOCKET_MAXTIME);
        }
    }
}

/**
 * handles a single client connection
 */
void Server::handleConnection(int client, int timeout) {
    auto start = std::chrono::steady_clock::now();
    // allocate stack buffers for host and service strings
    char host[NI_MAXHOST];
    char svc[NI_MAXSERV];

    // set all chars to null in each buffer
    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);
    
    // resolve a hostname if possible
    int result = getnameinfo((sockaddr*)&this->clientAddress, sizeof(this->clientAddress), host, NI_MAXHOST, svc, NI_MAXSERV, 0);

    // if hostname resolved, print it, else print the IP
    if (result) {
        std::cout << host << " connected on " << svc << std::endl;
    } else {
        inet_ntop(AF_INET, &this->clientAddress.sin_addr, host, NI_MAXHOST);
        std::cout << host << " connected on " << ntohs(this->clientAddress.sin_port) << std::endl;
    }

    // allocate a stack buffer for recieved data
    char buf[4096];

    // read and handle bytes until the connection ends
    while (true) {
        // set the recv buffer to null chars
        memset(buf, 0, 4096);

        // read data incoming from the client into the recv buffer
        int bytesRecv = recv(client, buf, 4096, 0);

        // get time spent on this thread
        auto stop = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(stop - start);

        // if it exceeds the limit, end the connection.
        if (duration.count() >= timeout) {
            std::cout << host << " timed out" << std::endl;
            goto srv_disconnect;
        }

        switch (bytesRecv) {
            case -1: // connection error
                std::cerr << "There was a connection issue with " << host << std::endl;
                goto srv_disconnect; // disconnect
                break;
            case 0: // client disconnected
                std::cout << host << " disconnected" << std::endl;
                goto srv_disconnect; // disconnect
                break;
            default:

                std::string bufStr(buf);
                Request r(bufStr);

                std::cout << host << " Requested " << r.Document << std::endl;

                std::string* response = this->handleRequest(r, client);
                send(client, response->c_str(), response->size(), 0);
                delete response;

                std::cout << "Ending connection to " << host << std::endl;
                goto srv_disconnect;
                break;
        }


    }

srv_disconnect:
    close(client);
}

std::string* Server::handleRequest(Request& req, int client)
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

        std::string connection = "Closed";

        // build response payload to return to client
        std::stringstream responseStream;
        responseStream << "HTTP/1.1 " << http << " " << status << "\n";
        responseStream << "Date: " << date << "\n";
        responseStream << "Server: " << SERVER_NAME << "\n";
        responseStream << "Last-Modified: " << date << "\n";
        responseStream << "Content-Length: " << body.size() + 1 << "\n";
        responseStream << "Content-Type: " << mime << "\n";
        responseStream << "Connection: " << connection << "\n";
        responseStream << "\n";
        responseStream << body << "\n";

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
void Server::serveDoc(std::string& document, const std::string& docRoot, std::vector<std::string>& docInfo)
{
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

    std::cout << "Serving document: " << p << std::endl;

    pthread_spin_lock(&this->spinlock); // lock resource

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
        docInfo.push_back("text/html"); // mime type is always text/html for now
        docInfo.push_back("200");
    } else {
        // file doesn't exist, throw a 404 and a default body
        docInfo.push_back("<h1>404: File Not found!</h1>");
        docInfo.push_back("text/html");
        docInfo.push_back("404");
    }

    pthread_spin_unlock(&this->spinlock); // unlock resource
}

// for fatal errors that should kill the program.
void Server::error(const std::string& message)
{
    std::cerr << message << std::endl;
    std::exit(1);
}

