#include "request.h"

/**
 * populates the Request class data members with request data
 */
Request::Request(const std::string& request)
    : isRequestGood(true)
{
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
    
    std::ostringstream oss;
    std::vector<std::string> splitLine;
    for (std::string& line : reqVec) {
        oss.clear();
        splitLine.clear();

        if (line.rfind("User-Agent:", 0) != std::string::npos) {
            this->splitString(line, ' ', splitLine);
            oss << splitLine[1];
            for (size_t n = 2; n < splitLine.size(); ++n) {
                oss << " " << splitLine[n];
            }
            this->UserAgent = oss.str();
        } else if (line.rfind("Host:", 0) != std::string::npos) {
            this->splitString(line, ' ', splitLine);
            if (splitLine.size() > 1) {
                this->Host = splitLine[1];
            }
        }

    }

    this->Method = requestLine[0];
    this->Document = this->DocumentOriginal = this->extractDocument(requestLine[1]);
    this->Version = requestLine[2];
    this->Body = reqVec[reqVec.size() - 1]; // the last line of the request
    this->Document = this->decodeURL(this->Document);
    this->GetParameters = this->extractGet(requestLine[1]);

    // everything except for the last 3 lines and the 1st line
    for (size_t i = 1; i < reqVec.size() - 3; ++i) {
        this->Headers.push_back(reqVec[i]);
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

/**
 * performs url decoding on str
 */
std::string Request::decodeURL(std::string& str)
{
    std::ostringstream oss;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%') {
            i++;
            std::string code(str.substr(i, 2));
            oss << static_cast<char>(std::strtol(code.c_str(), NULL, 16));
            i++;
        } else if (str[i] == '+') {
            oss << ' ';
        } else {
            oss << str[i];
        }
    }
    return oss.str();
}

/**
 * extract the string before the first '?' in the document line
 */
std::string Request::extractDocument(std::string& str) {
    std::ostringstream oss;
    for (size_t i = 0; str[i] && str[i] != '?'; ++i) {
        oss << str[i];
    }
    return oss.str();
}

/**
 * extract the string after the first '?' in the document line
 */
std::string Request::extractGet(std::string& str) {
    std::ostringstream oss;
    const char* s = str.c_str();

    // seek to the start of the GET parameters
    size_t i;
    for (i = 0; s[i] && s[i] != '?'; ++i)
        ;
    i++;

    // return the rest of the string
    for (; s[i]; ++i) {
        oss << s[i];
    }

    return oss.str();
}

