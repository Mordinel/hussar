#ifndef REQUEST_H
#define REQUEST_H

#include <iostream>
#include <string>
#include <chrono>
#include <vector>

class Request {
    private:

        void splitString(const std::string& str, char c, std::vector<std::string>& strVec);
    public:
        bool isRequestGood;
        std::string Method;
        std::string Document;
        std::string Version;
        std::vector<std::string> Headers;
        std::string Body;

        Request(const std::string& request);
};

#endif