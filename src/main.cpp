/**
*     Copyright (C) 2022 Mason Soroka-Gill
* 
*     This program is free software: you can redistribute it and/or modify
*     it under the terms of the GNU General Public License as published by
*     the Free Software Foundation, either version 3 of the License, or
*     (at your option) any later version.
* 
*     This program is distributed in the hope that it will be useful,
*     but WITHOUT ANY WARRANTY; without even the implied warranty of
*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*     GNU General Public License for more details.
* 
*     You should have received a copy of the GNU General Public License
*     along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <unistd.h> // for getopt
#include <iostream>
#include <string>

#include "hussar.h"

std::string DOCROOT = "";
bool VERBOSE = false;

void print_help(char* arg0)
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
    std::string document = req.document;

    if (req.is_good) {
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
                resp.headers["Content-Type"] = hus::get_mime(p);
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
        hus::print_lock.lock();
        for (auto& [key, value] : req.get) {
            std::cout << "GET[" << hus::strip_terminal_chars(key) << "] = " << hus::strip_terminal_chars(value) << "\n";
        }
        for (auto& [key, value] : req.post) {
            std::cout << "POST[" << hus::strip_terminal_chars(key) << "] = " << hus::strip_terminal_chars(value) << "\n";
        }
        hus::print_lock.unlock();
    }
}

int main(int argc, char* argv[])
{
    hus::Config config;
    config.host         = "127.0.0.1";
    config.port         = 8080;
    config.thread_count = 0; // 0 defaults to max hardware threads
    config.private_key  = "";
    config.certificate  = "";
    config.verbose      = false;

    bool config_changed = false;

    std::stringstream ss;

    int c;
    while ((c = getopt(argc, argv, "hvi:p:t:d:k:c:")) != -1) {
        switch (c) {

            case 'h':
                print_help(argv[0]);
                return 1;

            case 'v':
                config_changed = true;
                VERBOSE = true;
                config.verbose = true;
                break;

            case 'i':
                config_changed = true;
                config.host = optarg;
                break;

            case 'p':
                config_changed = true;
                ss.clear();
                ss << optarg;
                ss >> config.port;
                if (ss.fail()) {
                    std::cerr << "Error: " << optarg << " is not a valid port number, defaulting to (8080)\n";
                    config.port = 8080;
                }
                break;

            case 't':
                config_changed = true;
                ss.clear();
                ss << optarg;
                ss >> config.thread_count;
                if (ss.fail()) {
                    std::cerr << "Error: " << optarg << " is not a valid value for thread count, defaulting to (0)\n";
                    config.thread_count = 0;
                }
                break;
 
            case 'k':
                config_changed = true;
                config.private_key = optarg;
                break;

            case 'c':
                config_changed = true;
                config.certificate = optarg;
                break;

            case 'd':
                config_changed = true;
                DOCROOT = optarg;
                break;
       }
    }

    if (not config_changed) {
        print_help(argv[0]);
        return 1;
    }

    // if the paths are empty or one of the files don't exist or one of the files is not a regular file,
    //     use no ssl by setting the private key and certificate strings to empty strings.
    if (
        config.private_key == "" || config.certificate == "" ||
        not std::filesystem::exists(config.private_key) || not std::filesystem::exists(config.certificate) ||
        not std::filesystem::is_regular_file(config.private_key) || not std::filesystem::is_regular_file(config.certificate)
    ) {
        config.private_key = "";
        config.certificate = "";
    }

    hus::Hussar server(config);
    server.fallback(&web_server);
    server.serve();

    return 0;
}
