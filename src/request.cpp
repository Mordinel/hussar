#include "request.h"

/**
 * performs url decoding on str
 */
std::string UrlDecode(const std::string& str)
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
 * strips terminal control chars from a string
 */
std::string StripString(const std::string& str)
{
    std::ostringstream oss;

    for (char c : str) {
        switch (c) {
            case 0x07:
            case 0x08:
            case 0x09:
            case 0x0a:
            case 0x0b:
            case 0x0c:
            case 0x0d:
            case 0x1b:
            case 0x7f:
                break;
            default: [[likely]]
                oss << c;
                break;
        }
    }

    return oss.str();
}

/**
 * splits the string str into vector strVec, delimited by char c
 */
void SplitString(const std::string& str, char c, std::vector<std::string>& strVec)
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
 * populates the Request class data members with request data
 */
Request::Request(const std::string& request)
    : isRequestGood(true)
{
    // split into lines
    std::vector<std::string> reqVec;
    SplitString(request, '\n', reqVec);

    if (reqVec.size() < 1) {
        this->isRequestGood = false;
        return;
    }

    // get the first line as a vector
    std::vector<std::string> requestLine;
    SplitString(reqVec[0], ' ', requestLine);
        
    // invalid request line
    if (requestLine.size() != 3) {
        this->isRequestGood = false;
        return;
    }

    // parse the request line
    this->Method = requestLine[0];
    this->DocumentOriginal = requestLine[1];
    this->Version = requestLine[2];
    this->Document = this->extractDocument(this->DocumentOriginal);
    this->Document = UrlDecode(this->Document);
    this->GetParameters = this->extractGet(requestLine[1]);

    // TODO validate request line
    // this->validateRequestLine

    // parse the request headers and extract the request body
    size_t lineIdx;
    std::ostringstream oss;
    std::vector<std::string> splitLine;
    for (lineIdx = 1; lineIdx < reqVec.size(); ++lineIdx) {
        oss.clear();
        splitLine.clear();
        std::string& line = reqVec[lineIdx];

        if (line == "") {
            lineIdx++;
            for (; lineIdx < reqVec.size(); ++lineIdx) {
                line = reqVec[lineIdx];
                oss << line;
            }
            this->Body = oss.str();
            break;
        }

        if (line.rfind("User-Agent: ", 0) != std::string::npos) {
            this->UserAgent = this->extractHeaderContent(line);
        } else if (line.rfind("Host: ", 0) != std::string::npos) {
            this->Host = this->extractHeaderContent(line);
        }
    }
}

/**
 * extract the string after the host header name
 */
std::string Request::extractHeaderContent(std::string& str)
{
    std::ostringstream oss;
    std::vector<std::string> splitLine;
    size_t n;

    SplitString(str, ' ', splitLine);

    oss << splitLine[1];
    for (n = 2; n < splitLine.size(); ++n) {
        oss << " " << splitLine[n];
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

