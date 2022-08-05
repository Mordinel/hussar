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
#include "upload.h"
#include "cookie.h"
#include "session.h"

namespace hussar {
    class Request {

    public:
        bool is_good;
        bool keep_alive;
        std::string remote_host;
        std::string method;
        std::string document;
        std::string document_raw;
        std::string get_query_raw;
        std::string post_query_raw;
        std::string version;
        std::vector<std::string_view> headers;
        std::string user_agent;
        std::string connection;
        std::string content_type;
        std::string content_length;
        std::string virtual_host;
        std::string cookies_raw;
        std::string body;
        std::string session_id;
        std::unordered_map<std::string, std::string> get;
        std::unordered_map<std::string, std::string> post;
        std::unordered_map<std::string, Cookie> cookies;
        std::unordered_map<std::string, UploadedFile> files;

    private:
    
        /**
         * extract the string before the first '?' in the document line
         */
        std::string extract_document(std::string& str) {
            std::ostringstream oss;
            for (size_t i = 0; str[i] && str[i] != '?'; ++i) {
                oss << str[i];
            }
            return oss.str();
        }
        
        /**
         * extract the string after the first '?' in the document line
         */
        std::string extract_get(const std::string& str) {
            std::ostringstream oss;
            const char* s = str.c_str();
        
            // seek to the start of the GET parameters
            size_t i;
            for (i = 0; s[i] && s[i] != '?'; ++i)
                ;
            ++i;
        
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
        void parse_params(std::unordered_map<std::string, std::string>& dest, std::string& query_raw)
        {
            enum {
                PG_NAME, PG_VALUE
            } state = PG_NAME;
            std::istringstream iss(query_raw);
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
                            if (validate_param_name(name))
                                dest[name] = url_decode(value);
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
                                if (validate_param_name(name))
                                    dest[name] = url_decode(value);
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
        template <typename T>
        void collect_headers(std::vector<T>& headers)
        {
            for (auto& line : headers) {
                if (line.find("User-Agent: ", 0) != std::string::npos) {
                    this->user_agent = extract_header_content(line);
                } else if (line.find("Host: ", 0) != std::string::npos) {
                    this->virtual_host = extract_header_content(line);
                } else if (line.find("Connection: ", 0) != std::string::npos) {
                    this->connection = extract_header_content(line);
                } else if (line.find("Content-Type: ", 0) != std::string::npos) {
                    this->content_type = extract_header_content(line);
                } else if (line.find("Content-Length: ", 0) != std::string::npos) {
                    this->content_length = extract_header_content(line);
                } else if (line.find("Cookie: ", 0) != std::string::npos) {
                    this->cookies_raw = extract_header_content(line);
                }
            }
        }

        /**
         * validates the request line, returns true if it's valid, false if it's not.
         */
        template <typename T>
        bool validate_resource_line(std::vector<T>& request_line)
        {
            T http_version = request_line[2];
        
            auto http_version_split = split_string<T>(http_version, '/');
            if (http_version_split.size() != 2) {
                return false;
            }
        
            std::string_view protocol = http_version_split[0];
            if (protocol != "HTTP") {
                return false;
            }
        
            std::string http_version_trunc = http_version_split[1].substr(0,3);
            float version;
            std::istringstream iss{http_version_trunc};
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
        std::unordered_map<std::string, Cookie> get_cookies(const std::string& cookies_raw)
        {
            std::vector<Cookie> cookie_vec = deserialize_cookies(cookies_raw);
            std::unordered_map<std::string, Cookie> serialized_cookies;
            for (Cookie& cookie : cookie_vec) {
                if (cookie.name != "" && cookie.value != "") {
                    serialized_cookies[cookie.name] = cookie;
                }
            }

            return serialized_cookies;
        }

    public:
        /**
         * populates the Request class data members with request data
         */
        Request(const std::string& request, std::string host)
            : is_good(true), keep_alive(false), remote_host(host)
        {
            std::vector<std::string> headers;
            std::vector<std::string> resource_line_split;

            {
                // split headers and body content
                auto head_body_split = split_string<std::string>(request, "\r\n\r\n");
                if (head_body_split.size() < 1) {
                    this->is_good = false;
                    return;
                }

                // split headers by line
                headers = split_string<std::string>(head_body_split[0], "\r\n");
                if (headers.size() < 1) {
                    this->is_good = false;
                    return;
                }
                // remove the headers from the original split
                head_body_split.erase(head_body_split.begin());

                // join the remaining parts back into the body
                this->body = join_string(head_body_split, "\r\n\r\n");

                // get the first line as a vector and remove it from the headers
                resource_line_split = split_string<std::string>(headers[0], ' ');
                headers.erase(headers.begin());
            }

            // invalid request line
            if (resource_line_split.size() != 3) {
                this->is_good = false;
                return;
            }
        
            // parse the request line
            this->method = resource_line_split[0];
            this->document_raw = resource_line_split[1];
            this->version = resource_line_split[2];
            this->document = this->extract_document(this->document_raw);
            this->document = url_decode(this->document);
            this->get_query_raw = this->extract_get(std::string{resource_line_split[1]});
        
            // validate request line
            if (not this->validate_resource_line(resource_line_split)){
                this->is_good = false;
                return;
            }
        
            // parse the request headers
            this->collect_headers(headers);

            // parse cookie values
            this->cookies = this->get_cookies(this->cookies_raw);

            // set connection to keepalive
            this->keep_alive = this->connection == "keep-alive";

            // parse GET params
            this->parse_params(this->get, this->get_query_raw);

            // parse POST params if method is POST
            if (this->content_type == "application/x-www-form-urlencoded") {
                this->post_query_raw = this->body;
                this->parse_params(this->post, this->post_query_raw);
            }
        }

        // delete copy constructors
        Request(Request& req) = delete;
        Request(const Request& req) = delete;
        Request& operator=(Request& req) = delete;
        Request& operator=(const Request& req) = delete;
    };
}

