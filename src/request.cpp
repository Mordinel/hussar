#include "request.h"

/**
 * populates the Request class data members with request data
 */
Request::Request(const std::string& request)
    : isRequestGood(true)
{
    int i;
    // split into lines
    std::vector<std::string> reqVec;
    this->splitString(request, '\n', reqVec);

    if (reqVec.size() < 3) {
        isRequestGood = false;
        return;
    }

    // get the first line as a vector
    std::vector<std::string> requestLine;
    this->splitString(reqVec[0], ' ', requestLine);
        
    // invalid request line
    if (requestLine.size() != 3) {
        isRequestGood = false;
        return;
    }

    this->Method = requestLine[0];
    this->Document = requestLine[1];
    this->Version = requestLine[2];
    this->Body = reqVec[reqVec.size() - 1]; // the last line of the request

    // everything except for the last 3 lines and the 1st line
    for (i = 1; i < reqVec.size() - 3; ++i) {
        this->Headers.push_back(std::move(reqVec[i]));
    }
}

/**
 * splits the string str into vector strVec, delimited by char c
 */
void Request::splitString(const std::string& str, char c, std::vector<std::string>& strVec)
{
    std::string::size_type i = 0;
    std::string::size_type j = str.find(c);

    while (j != std::string::npos) {
        strVec.push_back(str.substr(i, j - i));
        i = ++j;
        j = str.find(c, j);
        if (j == std::string::npos) {
            strVec.push_back(str.substr(i, str.length()));
        }
    }
}
