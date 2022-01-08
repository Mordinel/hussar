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

#ifndef HUSSAR_ROUTER_H
#define HUSSAR_ROUTER_H

#include "hussar.h"

namespace hussar {

    void not_implemented(Request& req, Response& resp)
    {
        resp.code = "501";
        resp.status = "NOT IMPLEMENTED";
        resp.body = "<h1>501: Not Implemented!</h1>";
    }

    class Router {
    protected:
        void (*FALLBACK)(Request& req, Response& resp);
        std::unordered_map<std::string, void (*)(Request&, Response&)> GET;
        std::unordered_map<std::string, void (*)(Request&, Response&)> HEAD;
        std::unordered_map<std::string, void (*)(Request&, Response&)> POST;

    public:
        Router()
            : FALLBACK(&not_implemented)
        {}

        // delete copy constructors
        Router(Router& r) = delete;
        Router(const Router& r) = delete;
        Router& operator=(Router& r) = delete;
        Router& operator=(const Router& r) = delete;

        void route(Request& req, Response& resp)
        {
            if (req.is_good) {
                if (req.method == "GET") {
                    this->get(req, resp);
                } else if (req.method == "HEAD") {
                    this->head(req, resp);
                } else if (req.method == "POST") {
                    this->post(req, resp);
                } else {
                    this->fallback(req, resp);
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
            this->GET[route] = func;
        }

        // call get route
        void get(Request& req, Response& resp)
        {
            if (this->GET.find(req.document) != this->GET.end()) {
                (*this->GET[req.document])(req, resp);
            } else if (this->FALLBACK) {
                (*this->FALLBACK)(req, resp);
            } else {
                not_implemented(req, resp);
            }
        }

        // register head route
        void head(const std::string& route, void (*func)(Request& req, Response& resp))
        {
            this->HEAD[route] = func;
        }

        // call head route
        void head(Request& req, Response& resp)
        {
            if (this->HEAD.find(req.document) != this->HEAD.end()) {
                (*this->HEAD[req.document])(req, resp);
            } else if (this->FALLBACK) {
                (*this->FALLBACK)(req, resp);
            } else {
                not_implemented(req, resp);
            }
        }

        // register post route
        void post(const std::string& route, void (*func)(Request& req, Response& resp))
        {
            this->POST[route] = func;
        }

        // call post route
        void post(Request& req, Response& resp)
        {
            if (this->POST.find(req.document) != this->POST.end()) {
                (*this->POST[req.document])(req, resp);
            } else if (this->FALLBACK) {
                (*this->FALLBACK)(req, resp);
            } else {
                not_implemented(req, resp);
            }
        }

        // register fallback route
        void fallback(void (*func)(Request& req, Response& resp))
        {
            this->FALLBACK = func;
        }

        // call fallback route
        void fallback(Request& req, Response& resp)
        {
            if (this->FALLBACK) {
                (*this->FALLBACK)(req, resp);
            }
        }
    };
};

#endif
