#ifndef HUSSAR_RESPONSE_H
#define HUSSAR_RESPONSE_H

#include "pch.h"
#include "request.h"

namespace hussar {
    std::unordered_map<std::string, std::string> statuses = {
        { "200", "OK" },
        { "201", "CREATED" },
        { "202", "ACCEPTED" },
        { "204", "NO CONTENT" },
        { "300", "MULTIPLE CHOICES" },
        { "301", "MOVED PERMANENTLY" },
        { "302", "FOUND" },
        { "303", "SEE OTHER" },
        { "304", "NOT MODIFIED" },
        { "305", "USE PROXY" },
        { "307", "TEMPORARY REDIRECT" },
        { "308", "PERMANENT REDIRECT" },
        { "400", "BAD REQUEST" },
        { "401", "UNAUTHORIZED" },
        { "402", "PAYMENT REQUIRED" },
        { "403", "FORBIDDEN" },
        { "404", "NOT FOUND" },
        { "408", "REQUEST TIMEOUT" },
        { "410", "GONE" },
        { "411", "LENGTH REQUIRED" },
        { "412", "PRECONDITION FAILED" },
        { "413", "PAYLOAD TOO LARGE" },
        { "414", "URI TOO LONG" },
        { "415", "UNSUPPORTED MEDIA TYPE" },
        { "418", "I AM A TEAPOT" },
        { "426", "UPGRADE REQUIRED" },
        { "429", "TOO MANY REQUESTS" },
        { "451", "UNAVAILABLE FOR LEGAL REASONS" },
        { "500", "INTERNAL SERVER ERROR" },
        { "501", "NOT IMPLEMENTED" },
        { "502", "BAD GATEWAY" },
        { "503", "SERVICE UNAVAILABLE" },
    };

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

        // transform field data into an HTTP response
        std::string Serialize()
        {
            std::ostringstream responseStream;
            std::ostringstream bodyLengthStream;

            // code status texts
            if (statuses.find(this->code) != statuses.end()) {
                responseStream << this->proto << " " << this->code << " " << statuses[this->code] << "\n";
            } else if (this->status != "") {
                responseStream << this->proto << " " << this->code << " " << this->status << "\n";
            } else { // code not implemented and custom status is empty
                this->code = "500";
                responseStream << this->proto << " " << this->code << " " << statuses[this->code] << "\n";
                this->Headers["Content-Type"] = "text/html";
                this->body = "<h1>500: " + statuses[this->code] + "</h1>";
            }

            // content length header
            if (requestMethod != "HEAD") {
                bodyLengthStream << body.size();
                this->Headers["Content-Length"] = bodyLengthStream.str();
            } else {
                bodyLengthStream << 0;
                this->Headers["Content-Length"] = bodyLengthStream.str();
            }

            // stored headers
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
