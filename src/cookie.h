#ifndef HUSSAR_COOKIE_H
#define HUSSAR_COOKIE_H

#include "pch.h"

namespace hussar {
    enum class samesite {
        UNDEFINED,
        NONE,
        LAX,
        STRICT
    };

    struct Cookie {
        std::string name;
        std::string value;
        bool Secure;
        bool HttpOnly;
        samesite SameSite;
        std::string Domain;
        std::string Path;
        int MaxAge;

        Cookie()
            : name(""), value(""), Secure(false), HttpOnly(false), SameSite(samesite::UNDEFINED), Domain(""), Path(""), MaxAge(-1)
        {}

        Cookie(const std::string& name, const std::string& value)
            : name(name), value(value), Secure(false), HttpOnly(false), SameSite(samesite::UNDEFINED), Domain(""), Path(""), MaxAge(-1)
        {}
    };

    std::vector<Cookie> ParseCookies(const std::string& cookieStr)
    {
        enum {
            C_NAME, C_VALUE, C_SAMESITE, C_DOMAIN, C_PATH, C_EXPIRES, C_MAXAGE
        } state = C_NAME;
        std::vector<Cookie> cookie_vec;
        std::istringstream iss(cookieStr);
        std::ostringstream oss;
        std::stringstream ss;
        std::string str;

        char c;
        Cookie cookie;
        for (iss >> c ;; iss >> c) {
            if (iss.eof()) {
                switch (state) {
                    default: break;
                    case C_VALUE:
                        str = StripString(oss.str());
                        if (str.size()) {
                            cookie.value = str;
                            cookie_vec.push_back(cookie);
                        }
                        break;
                    case C_NAME:
                        str = StripString(oss.str());
                        if (str == "Secure") {
                            cookie.Secure = true;
                            cookie_vec.push_back(cookie);
                        } else if (str == "HttpOnly") {
                            cookie.HttpOnly = true;
                            cookie_vec.push_back(cookie);
                        } else {
                            cookie_vec.push_back(cookie);
                        }
                        break;
                    case C_SAMESITE:
                        str = StripString(oss.str());
                        if (str == "None") {
                            cookie.SameSite = samesite::NONE;
                        } else if (str == "Lax") {
                            cookie.SameSite = samesite::LAX;
                        } else if (str == "Strict") {
                            cookie.SameSite = samesite::STRICT;
                        } else {
                            cookie.SameSite = samesite::UNDEFINED;
                        }
                        cookie_vec.push_back(cookie);
                        break;
                    case C_DOMAIN:
                        cookie.Domain = StripString(oss.str());
                        cookie_vec.push_back(cookie);
                        break;
                    case C_PATH:
                        cookie.Path = StripString(oss.str());
                        cookie_vec.push_back(cookie);
                        break;
                    case C_EXPIRES:
                        // don't use this feature
                        break;
                    case C_MAXAGE:
                        ss.clear();
                        ss << StripString(oss.str());
                        ss >> cookie.MaxAge;
                        if (ss.fail()) {
                            cookie.MaxAge = -1;
                        }
                        cookie_vec.push_back(cookie);
                        break;
                }
                break;
            }
            
            switch (c) {
                case '=':
                c_equal:
                    str = oss.str();
                    if (str.size()) {
                        str = StripString(str);
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
                                cookie_vec.push_back(cookie);
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
                            cookie.value = StripString(str);
                        }
                        oss.str("");
                        break;
                    } else if (state == C_NAME) {
                        str = StripString(oss.str());
                        if (str == "Secure") {
                            cookie.Secure = true;
                        } else if (str == "HttpOnly") {
                            cookie.HttpOnly = true;
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
                    c_other:
                        switch (state) {
                            default:
                                break;
                            case C_SAMESITE:
                                str = StripString(oss.str());
                                if (str == "None") {
                                    cookie.SameSite = samesite::NONE;
                                } else if (str == "Lax") {
                                    cookie.SameSite = samesite::LAX;
                                } else if (str == "Strict") {
                                    cookie.SameSite = samesite::STRICT;
                                } else {
                                    cookie.SameSite = samesite::UNDEFINED;
                                }
                                break;
                            case C_DOMAIN:
                                cookie.Domain = StripString(oss.str());
                                break;
                            case C_PATH:
                                cookie.Path = StripString(oss.str());
                                break;
                            case C_EXPIRES:
                                // don't use this feature
                                break;
                            case C_MAXAGE:
                                ss.clear();
                                ss << StripString(oss.str());
                                ss >> cookie.MaxAge;
                                if (ss.fail()) {
                                    cookie.MaxAge = -1;
                                }
                                break;
                        }
                        state = C_NAME;
                        oss.str("");
                        break;
                }
            }
        }

        return cookie_vec;
    }
};

#endif
