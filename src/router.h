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
        void (*fallback)(Request& req, Response& resp);
        std::unordered_map<std::string, void (*)(Request&, Response&)> get;
        std::unordered_map<std::string, void (*)(Request&, Response&)> head;
        std::unordered_map<std::string, void (*)(Request&, Response&)> post;

    public:
        void registerDefault()
        {
            this->get["/"]  = this->fallback;
            this->head["/"] = this->fallback;
            this->post["/"] = this->fallback;
        }

        Router()
            : fallback(&not_implemented)
        {
            this->registerDefault();
        }

        void Route(Request& req, Response& resp)
        {
            if (req.isGood) {
                if (req.Method == "GET") {
                    this->GET(req, resp);
                } else if (req.Method == "HEAD") {
                    this->HEAD(req, resp);
                } else if (req.Method == "POST") {
                    this->POST(req, resp);
                } else {
                    this->DEFAULT(req, resp);
                }
            } else {
                resp.Headers["Content-Type"] = "text/html";
                resp.code = "400";
                resp.body = "<h1>400: Bad Request</h1>";
            }
        }

        // register GET route
        void GET(const std::string& route, void (*func)(Request& req, Response& resp))
        {
            this->get[route] = func;
        }

        // call GET route
        void GET(Request& req, Response& resp)
        {
            if (this->get.find(req.Document) != this->get.end()) {
                (*this->get[req.Document])(req, resp);
            } else if (this->fallback) {
                (*this->fallback)(req, resp);
            } else {
                not_implemented(req, resp);
            }
        }

        // register HEAD route
        void HEAD(const std::string& route, void (*func)(Request& req, Response& resp))
        {
            this->head[route] = func;
        }

        // call HEAD route
        void HEAD(Request& req, Response& resp)
        {
            if (this->head.find(req.Document) != this->head.end()) {
                (*this->head[req.Document])(req, resp);
            } else if (this->fallback) {
                (*this->fallback)(req, resp);
            } else {
                not_implemented(req, resp);
            }
        }

        // register POST route
        void POST(const std::string& route, void (*func)(Request& req, Response& resp))
        {
            this->post[route] = func;
        }

        // call POST route
        void POST(Request& req, Response& resp)
        {
            if (this->post.find(req.Document) != this->post.end()) {
                (*this->post[req.Document])(req, resp);
            } else if (this->fallback) {
                (*this->fallback)(req, resp);
            } else {
                not_implemented(req, resp);
            }
        }

        // register DEFAULT route
        void DEFAULT(void (*func)(Request& req, Response& resp))
        {
            this->fallback = func;
            this->registerDefault();
        }

        // call DEFAULT route
        void DEFAULT(Request& req, Response& resp)
        {
            if (this->fallback) {
                (*this->fallback)(req, resp);
            }
        }
    };
};

#endif
