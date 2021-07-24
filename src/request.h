#ifndef HUSSAR_REQUEST_H
#define HUSSAR_REQUEST_H

#include "util.h"

namespace hussar {
    class Request {
    private:
    
        /**
         * extract the string after the host header name
         */
        std::string extractHeaderContent(std::string& str)
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
        std::string extractDocument(std::string& str) {
            std::ostringstream oss;
            for (size_t i = 0; str[i] && str[i] != '?'; ++i) {
                oss << str[i];
            }
            return oss.str();
        }
        
        /**
         * extract the string after the first '?' in the document line
         */
        std::string extractGet(std::string& str) {
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
    
        /**
         * validates the request line, returns true if it's valid, false if it's not.
         */
        bool validateRequestLine(std::vector<std::string>& requestLine)
        {
            std::string& method   = requestLine[0];
            std::string& httpver  = requestLine[2];
        
            if ((method != "GET") && (method != "POST")) {
                return false;
            }
        
            std::vector<std::string> httpvervec;
            SplitString(httpver, '/', httpvervec);
            if (httpvervec.size() != 2) {
                return false;
            }
        
            std::string& protocol = httpvervec[0];
            if (protocol != "HTTP") {
                return false;
            }
        
            std::string strversion = httpvervec[1].substr(0,3);
            float version;
            std::istringstream iss(strversion);
            iss >> version;
            if (iss.fail()) {
                return false;
            }
        
            if ((version < 0.9f) || (version > 1.2f)) {
                return false;
            }
        
            return true;
        }
    
    
    public:

        bool isRequestGood;
        std::string Method;
        std::string Document;
        std::string DocumentOriginal;
        std::string GetParameters;
        std::string PostParameters;
        std::string Version;
        std::vector<std::string> Headers;
        std::string UserAgent;
        std::string Host;
        std::string Body;
    
        /**
         * populates the Request class data members with request data
         */
        Request(const std::string& request)
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
        
            // validate request line
            if (not this->validateRequestLine(requestLine)){
                this->isRequestGood = false;
                return;
            }
        
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
    };
}

#endif
