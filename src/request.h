#ifndef HUSSAR_REQUEST_H
#define HUSSAR_REQUEST_H

#include "pch.h"
#include "cookie.h"

namespace hussar {
    class Request {

    public:
        bool isGood;
        bool KeepAlive;
        std::string RemoteHost;
        std::string Method;
        std::string Document;
        std::string DocumentOriginal;
        std::string GetParameters;
        std::string PostParameters;
        std::string Version;
        std::vector<std::string> Headers;
        std::string UserAgent;
        std::string Connection;
        std::string ContentType;
        std::string VirtualHost;
        std::string CookieString;
        std::string Body;
        std::unordered_map<std::string, std::string> GET;
        std::unordered_map<std::string, std::string> POST;
        std::unordered_map<std::string, Cookie> Cookies;

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
        
            return TerminalString(oss.str());
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
         * parses the given parameters in getStr and stores them in dest
         * handles both GET and POST parameters
         */
        void parseParams(std::unordered_map<std::string, std::string>& dest, std::string& getStr)
        {
            enum {
                PG_NAME, PG_VALUE
            } state = PG_NAME;
            std::istringstream iss(getStr);
            std::ostringstream oss;
            std::string name;
            std::string value;

            char c;
            for (iss >> c ;; iss >> c) { // extract from the istream into c
                if (iss.eof()) { // EXIT CASE
                    // store the last parameter if that needs to happen
                    if (state == PG_VALUE && name.size()) {
                        value = oss.str();
                        if (value.size()) {
                            if (ValidateParamName(name))
                                dest[name] = UrlDecode(value);
                        }
                    }
                    break;
                }

                // state machine parser consumes 'c' and populates 'name' and 'value', then inserts the pair into the hashmap
                switch(c) {
                    case '=':
                        if (state == PG_VALUE) {
                            goto pg_default;
                        }
                        name = oss.str();
                        if (name.size()) {
                            oss.str("");
                            state = PG_VALUE;
                        } else {
                            name = "";
                        }
                        break;
                    case '&':
                        if (name.size()) {
                            value = oss.str();
                            if (value.size()) {
                                oss.str("");
                                if (ValidateParamName(name))
                                    dest[name] = UrlDecode(value);
                                state = PG_NAME;
                            } else {
                                value = "";
                            }
                        }
                        break;
                    default:
                    pg_default:
                        oss << c;
                        break;
                }
            }
        }

        /**
         * collects header parameters
         * returns the line that the body should be on.
         */
        size_t collectHeaders(std::vector<std::string>& reqVec)
        {
            // parse the request headers and extract the request body
            size_t lineIdx;
            std::vector<std::string> splitLine;
            for (lineIdx = 1; lineIdx < reqVec.size(); ++lineIdx) {
                splitLine.clear();
                std::string& line = reqVec[lineIdx];
        
                if (line == "") {
                    lineIdx++;
                    break;
                }
        
                if (line.rfind("User-Agent: ", 0) != std::string::npos) {
                    this->UserAgent = this->extractHeaderContent(line);
                } else if (line.rfind("Host: ", 0) != std::string::npos) {
                    this->VirtualHost = this->extractHeaderContent(line);
                } else if (line.rfind("Connection: ", 0) != std::string::npos) {
                    this->Connection = this->extractHeaderContent(line);
                } else if (line.rfind("Content-Type: ", 0) != std::string::npos) {
                    this->ContentType = this->extractHeaderContent(line);
                } else if (line.rfind("Cookie: ", 0) != std::string::npos) {
                    this->CookieString = this->extractHeaderContent(line);
                }
            }

            return lineIdx-1;
        }

        /**
         * extracts the body of the document, assuming lineIdx is the line in reqVec that the body starts on
         */
        std::string extractBody(std::vector<std::string>& reqVec, size_t index)
        {
            size_t lineIdx = index;
            std::ostringstream oss;
            for (; lineIdx < reqVec.size(); ++lineIdx) {
                oss << reqVec[lineIdx];
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
        
            if ((method != "GET") && (method != "POST") && (method != "HEAD")) {
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

        /**
         * returns an unordered map of http cookies with the minimum valid values
         */
        std::unordered_map<std::string, Cookie> getCookies(const std::string& cookieString)
        {
            std::vector<Cookie> cookie_vec = ParseCookies(cookieString);
            std::unordered_map<std::string, Cookie> cookie_map;
            for (Cookie& cookie : cookie_vec) {
                if (cookie.name != "" && cookie.value != "") {
                    cookie_map[cookie.name] = cookie;
                }
            }

            return cookie_map;
        }

    public:
        /**
         * populates the Request class data members with request data
         */
        Request(const std::string& request, std::string host)
            : isGood(true), KeepAlive(false), RemoteHost(host)
        {
            // split into lines
            std::vector<std::string> reqVec;
            SplitString(request, '\n', reqVec);
        
            if (reqVec.size() < 1) {
                this->isGood = false;
                return;
            }
        
            // get the first line as a vector
            std::vector<std::string> requestLine;
            SplitString(reqVec[0], ' ', requestLine);
                
            // invalid request line
            if (requestLine.size() != 3) {
                this->isGood = false;
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
                this->isGood = false;
                return;
            }
        
            // parse the request headers
            size_t lineIdx = this->collectHeaders(reqVec);

            // parse cookie values
            this->Cookies = this->getCookies(this->CookieString);

            // set connection to keepalive
            this->KeepAlive = this->Connection == "keep-alive";

            // extract the request body
            this->Body = this->extractBody(reqVec, lineIdx);

            // parse GET params
            this->parseParams(this->GET, this->GetParameters);

            // parse POST params if method is POST
            if (this->Method == "POST" && lineIdx < reqVec.size()) {
                this->PostParameters = reqVec[lineIdx];
                this->parseParams(this->POST, this->PostParameters);
            }
        }
    };
}

#endif
