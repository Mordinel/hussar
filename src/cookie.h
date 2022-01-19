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

namespace hussar {
    enum class SameSite {
        UNDEFINED,
        NONE,
        LAX,
        STRICT
    };

    struct Cookie {
        std::string name;
        std::string value;
        bool secure;
        bool http_only;
        SameSite same_site;
        std::string domain;
        std::string path;
        int max_age;

        Cookie()
            : name(""), value(""), secure(false), http_only(false), same_site(SameSite::UNDEFINED), domain(""), path(""), max_age(-1)
        {}

        /**
         * returns true if the cookie is valid
         */
        bool is_valid()
        {
            if (not validate_param_name(trim(this->name))) {
                return false;
            }

            if (not trim(this->value).size()) {
                return false;
            }

            return true;
        }

        /**
         * returns the cookie in its http string representation
         */
        std::string serialize()
        {
            std::ostringstream oss;

            // check if cookie is valid
            if (not this->is_valid()) {
                return "";
            }

            std::string name_stripped = trim(this->name);
            std::string value_stripped = trim(this->value);
            oss << name_stripped << "=" << value_stripped;

            std::string domain_stripped = trim(this->domain);
            if (domain_stripped.size()) {
                oss << "; Domain=" << domain_stripped;
            }
            
            std::string path_stripped = trim(this->path);
            if (path_stripped.size()) {
                oss << "; Path=" << path_stripped;
            }

            if (this->same_site != SameSite::UNDEFINED) {
                oss << "; SameSite=";
                switch (this->same_site) {
                    default: break;
                    case SameSite::NONE:
                        oss << "None";
                        break;
                    case SameSite::LAX:
                        oss << "Lax";
                        break;
                    case SameSite::STRICT:
                        oss << "Strict";
                        break;
                }
            }

            if (this->max_age > -1) {
                oss << "; Max-Age=" << this->max_age;
            }

            if (this->secure) {
                oss << "; Secure";
            }

            if (this->http_only) {
                oss << "; HttpOnly";
            }

            return oss.str();
        }
    };

    /**
     * Take a vector of Cookie structs and serialize them into HTTP cookie strings
     */
    std::string serialize_cookies(std::vector<Cookie>& cookies)
    {
        std::ostringstream oss;
        std::string serialized;
        bool first = true;

        // input has no cookies
        if (not cookies.size()) {
            return "";
        }

        // for ever cookie in vec
        for (Cookie& cookie : cookies) {
            // serialize it
            serialized = cookie.serialize();
            // if the result contains something
            if (serialized.size()) {
                // put the serialized cookie into the output buffer
                if (first) {
                    oss << serialized;
                    first = false;
                } else {
                    oss << "; " << serialized;
                }
            }
        }

        return oss.str();
    }

    /**
     * Take a string containing arbitrary HTTP cookies and parse them into Cookie structs
     */
    std::vector<Cookie> deserialize_cookies(const std::string& cookies)
    {
        enum {
            C_NAME, C_VALUE, C_SAMESITE, C_DOMAIN, C_PATH, C_EXPIRES, C_MAXAGE
        } state = C_NAME;
        std::vector<Cookie> serialize_cookies;
        std::istringstream iss(cookies);
        std::ostringstream oss;
        std::stringstream ss;
        std::string str;

        // state machine parser for http cookie strings
        char c;
        Cookie cookie;
        for (iss >> c ;; iss >> c) {
            // wrap up parsing
            if (iss.eof()) {
                switch (state) {
                    default: break;
                    case C_VALUE:
                        str = trim(oss.str());
                        if (str.size()) {
                            cookie.value = str;
                            serialize_cookies.push_back(cookie);
                        }
                        break;
                    case C_NAME:
                        str = trim(oss.str());
                        if (str == "Secure") {
                            cookie.secure = true;
                            serialize_cookies.push_back(cookie);
                        } else if (str == "HttpOnly") {
                            cookie.http_only = true;
                            serialize_cookies.push_back(cookie);
                        } else {
                            serialize_cookies.push_back(cookie);
                        }
                        break;
                    case C_SAMESITE:
                        str = trim(oss.str());
                        if (str == "None") {
                            cookie.same_site = SameSite::NONE;
                        } else if (str == "Lax") {
                            cookie.same_site = SameSite::LAX;
                        } else if (str == "Strict") {
                            cookie.same_site = SameSite::STRICT;
                        } else {
                            cookie.same_site = SameSite::UNDEFINED;
                        }
                        serialize_cookies.push_back(cookie);
                        break;
                    case C_DOMAIN:
                        cookie.domain = trim(oss.str());
                        serialize_cookies.push_back(cookie);
                        break;
                    case C_PATH:
                        cookie.path = trim(oss.str());
                        serialize_cookies.push_back(cookie);
                        break;
                    case C_EXPIRES:
                        // don't use this feature
                        break;
                    case C_MAXAGE:
                        ss.clear();
                        ss << trim(oss.str());
                        ss >> cookie.max_age;
                        if (ss.fail()) {
                            cookie.max_age = -1;
                        }
                        serialize_cookies.push_back(cookie);
                        break;
                }
                break;
            }
            
            // consume one char from the http cookie string
            switch (c) {
                case '=':
                c_equal:
                    str = oss.str();
                    if (str.size()) {
                        str = trim(str);
                        // could be parameter options
                        if (str == "SameSite") {
                            state = C_SAMESITE;
                        } else if (str == "Domain") {
                            state = C_DOMAIN;
                        } else if (str == "Path") {
                            state = C_PATH;
                        } else if (str == "Expires") {
                            state = C_EXPIRES;
                        } else if (str == "Max-Age") {
                            state = C_MAXAGE;
                        } else if (state == C_NAME) {
                            if (cookie.name == "" && cookie.value == "") {
                                oss.str("");
                                state = C_VALUE;
                                cookie.name = str;
                            } else {
                                serialize_cookies.push_back(cookie);
                                // start a new cookie
                                cookie = Cookie();
                                state = C_NAME;
                                goto c_equal;
                            }
                            break;
                        } else if (state == C_VALUE) {
                            goto c_default;
                        }
                    }
                    oss.str("");
                    break;

                case ';':
                    if (state == C_VALUE) {
                        str = oss.str();
                        if (str.size()) {
                            oss.str("");
                            state = C_NAME;
                            cookie.value = trim(str);
                        }
                        oss.str("");
                        break;
                    } else if (state == C_NAME) {
                        str = trim(oss.str());
                        if (str == "Secure") {
                            cookie.secure = true;
                        } else if (str == "HttpOnly") {
                            cookie.http_only = true;
                        }
                        oss.str("");
                        break;
                    }
                    goto c_other;
                    break;

                default: {
                    c_default:
                        oss << c;
                        break;

                    // custom case for cookie attributes with parameters
                    c_other:
                        switch (state) {
                            default:
                                break;
                            case C_SAMESITE:
                                str = trim(oss.str());
                                if (str == "None") {
                                    cookie.same_site = SameSite::NONE;
                                } else if (str == "Lax") {
                                    cookie.same_site = SameSite::LAX;
                                } else if (str == "Strict") {
                                    cookie.same_site = SameSite::STRICT;
                                } else {
                                    cookie.same_site = SameSite::UNDEFINED;
                                }
                                break;
                            case C_DOMAIN:
                                cookie.domain = trim(oss.str());
                                break;
                            case C_PATH:
                                cookie.path = trim(oss.str());
                                break;
                            case C_EXPIRES:
                                // don't use this feature
                                break;
                            case C_MAXAGE:
                                ss.clear();
                                ss << trim(oss.str());
                                ss >> cookie.max_age;
                                if (ss.fail()) {
                                    cookie.max_age = -1;
                                }
                                break;
                        }
                        state = C_NAME;
                        oss.str("");
                        break;
                }
            }
        }

        return serialize_cookies;
    }
};

