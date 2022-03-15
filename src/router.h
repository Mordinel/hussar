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

#include "hussar.h"

namespace hussar {
    using handler = std::function<void (Request&, Response&)>;
    void not_implemented(Request& req, Response& resp)
    {
        resp.code = "501";
        resp.status = "NOT IMPLEMENTED";
        resp.body = "<h1>501: Not Implemented!</h1>";
    }

    class Router {
    protected:
        handler FALLBACK;
        std::unordered_map<std::string, handler> GET;
        std::unordered_map<std::string, handler> HEAD;
        std::unordered_map<std::string, handler> POST;

    public:
        Router()
            : FALLBACK(&not_implemented)
        {}

        // delete copy constructors
        Router(Router& r) = delete;
        Router(const Router& r) = delete;
        Router& operator=(Router& r) = delete;
        Router& operator=(const Router& r) = delete;

        void route(Request& req, Response& resp) const
        {
            if (req.is_good) {
                if (req.method == "GET") {
                    get(req, resp);
                } else if (req.method == "HEAD") {
                    head(req, resp);
                } else if (req.method == "POST") {
                    post(req, resp);
                } else {
                    fallback(req, resp);
                }
            } else {
                resp.headers["Content-Type"] = "text/html";
                resp.code = "400";
                resp.body = "<h1>400: Bad Request</h1>";
            }
        }

        // register get route
        void get(const std::string& route, void (*func)(Request& req, Response& resp))
        {
            GET[route] = func;
        }

        // call get route
        void get(Request& req, Response& resp) const
        {
            if (GET.find(req.document) != GET.end()) {
                GET.at(req.document)(req, resp);
            } else if (FALLBACK) {
                FALLBACK(req, resp);
            } else {
                not_implemented(req, resp);
            }
        }

        // register head route
        void head(const std::string& route, void (*func)(Request& req, Response& resp))
        {
            HEAD[route] = func;
        }

        // call head route
        void head(Request& req, Response& resp) const
        {
            if (HEAD.contains(req.document)) {
                HEAD.at(req.document)(req, resp);
            } else if (FALLBACK) {
                FALLBACK(req, resp);
            } else {
                not_implemented(req, resp);
            }
        }

        // register post route
        void post(const std::string& route, handler func)
        {
            POST[route] = func;
        }

        // call post route
        void post(Request& req, Response& resp) const
        {
            if (POST.contains(req.document)) {
                POST.at(req.document)(req, resp);
            } else if (FALLBACK) {
                FALLBACK(req, resp);
            } else {
                not_implemented(req, resp);
            }
        }

        // register fallback route
        void fallback(handler func)
        {
            FALLBACK = func;
        }

        // call fallback route
        void fallback(Request& req, Response& resp) const
        {
            if (FALLBACK) {
                FALLBACK(req, resp);
            }
        }
    };
};

