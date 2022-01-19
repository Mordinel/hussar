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

#pragma once

#include "libs.h"
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
        Request& request;

    public:
        std::unordered_map<std::string, std::string> headers;
        std::vector<Cookie> cookies;
        std::string proto;
        std::string code;
        std::string status;
        std::string body; 

        Response(Request& req)
            : request(req), proto("HTTP/1.1"), code("200"), status("OK")
        {
            // get local time
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto local_time = *std::localtime(&time);
            
            // format date for http output
            std::ostringstream date_stream;
            date_stream << std::put_time(&local_time, "%a, %d %b %Y %H:%M:%S");
            std::string date = date_stream.str();

            this->headers["Date"] = date;
            this->headers["Server"] = SERVER_NAME;
            this->headers["Connection"] = req.keep_alive ? "keep-alive" : "close";
            this->headers["Content-Type"] = "text/html";

            bool update_session = false;
            // get session id
            if (req.cookies.find("id") != req.cookies.end()) {
                req.session_id = req.cookies["id"].value; 
                if (not session_exists(req.session_id)) {
                    req.session_id = create_session();
                    update_session = true;
                }
            } else {
                req.session_id = create_session();
                update_session = true;
            }

            // if new session, create it
            if (update_session) {
                this->cookies.emplace_back(Cookie{});
                Cookie& c = this->cookies[this->cookies.size()-1];
                c.name = "id";
                c.value = req.session_id;
                c.http_only = true;
            }
        }

        // delete copy constructors
        Response(Response& resp) = delete;
        Response(const Response& resp) = delete;
        Response& operator=(Response& resp) = delete;
        Response& operator=(const Response& resp) = delete;

        // transform field data into an HTTP response
        std::string serialize()
        {
            std::ostringstream response_stream;

            // code status texts
            if (statuses.find(this->code) != statuses.end()) {
                response_stream << this->proto << " " << this->code << " " << statuses[this->code] << "\n";
            } else if (this->status != "") {
                response_stream << this->proto << " " << this->code << " " << this->status << "\n";
            } else { // code not implemented and custom status is empty
                this->code = "500";
                response_stream << this->proto << " " << this->code << " " << statuses[this->code] << "\n";
                this->headers["Content-Type"] = "text/html";
                this->body = "<h1>500: " + statuses[this->code] + "</h1>";
            }

            // stored headers
            for (auto& [key, data] : this->headers) {
                if (key != "") {
                    response_stream << key << ": " << data << "\n";
                }
            }

            // set cookie headers
            for (Cookie& cookie : this->cookies) {
                if (cookie.is_valid()) {
                    response_stream << "Set-Cookie: " << cookie.serialize() << "\n";
                }
            }

            // content length header
            if (this->request.method != "HEAD") {
                response_stream << "Content-Length: " << body.size() << "\n";
            } else {
                response_stream << "Content-Length: 0\n";
            }

            response_stream << "\n";

            if (this->request.method != "HEAD") {
                response_stream << this->body;
            }

            return response_stream.str();
        }
    };
};

