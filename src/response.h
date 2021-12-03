#ifndef HUSSAR_RESPONSE_H
#define HUSSAR_RESPONSE_H

#include "pch.h"
#include "request.h"

namespace hussar {
    class Response {
    private:
        std::string requestMethod;

    public:
        std::unordered_map<std::string, std::string> Headers;
        std::string proto;
        std::string code;
        std::string status;
        std::string body; 

        Response(Request& req)
            : requestMethod(req.Method), proto("HTTP/1.1"), code("200"), status("OK")
        {
            // get local time
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto localTime = *std::localtime(&time);
            
            // format date for http output
            std::ostringstream dateStream;
            dateStream << std::put_time(&localTime, "%a, %d %b %Y %H:%M:%S");
            std::string date = dateStream.str();

            this->Headers["Date"] = date;
            this->Headers["Server"] = SERVER_NAME;
            this->Headers["Connection"] = req.KeepAlive ? "keep-alive" : "close";
            this->Headers["Content-Type"] = "text/html";
        }

        std::string Serialize()
        {
            std::ostringstream responseStream;
            std::ostringstream bodyLengthStream;

            responseStream << this->proto << " " << this->code << " " << this->status << "\n";

            if (requestMethod != "HEAD") {
                bodyLengthStream << body.size();
                this->Headers["Content-Length"] = bodyLengthStream.str();
            } else {
                bodyLengthStream << 0;
                this->Headers["Content-Length"] = bodyLengthStream.str();
            }

            for (auto& [key, data] : this->Headers) {
                if (key != "") {
                    responseStream << key << ": " << data << "\n";
                }
            }

            responseStream << "\n";

            if (requestMethod != "HEAD") {
                responseStream << this->body;
            }
            return responseStream.str();
        }
    };
};

#endif
