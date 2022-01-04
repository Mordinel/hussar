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

        /**
         * returns true if the cookie is valid
         */
        bool IsValid()
        {
            if (not ValidateParamName(StripString(this->name))) {
                return false;
            }

            if (not StripString(this->value).size()) {
                return false;
            }

            return true;
        }

        /**
         * returns the cookie in its http string representation
         */
        std::string Serialize()
        {
            std::ostringstream oss;

            // check if cookie is valid
            if (not this->IsValid()) {
                return "";
            }

            std::string name_stripped = StripString(this->name);
            std::string value_stripped = StripString(this->value);
            oss << name_stripped << "=" << value_stripped;

            std::string domain_stripped = StripString(this->Domain);
            if (domain_stripped.size()) {
                oss << "; Domain=" << domain_stripped;
            }
            
            std::string path_stripped = StripString(this->Path);
            if (path_stripped.size()) {
                oss << "; Path=" << path_stripped;
            }

            if (this->SameSite != samesite::UNDEFINED) {
                oss << "; SameSite=";
                switch (this->SameSite) {
                    default: break;
                    case samesite::NONE:
                        oss << "None";
                        break;
                    case samesite::LAX:
                        oss << "Lax";
                        break;
                    case samesite::STRICT:
                        oss << "Strict";
                        break;
                }
            }

            if (this->MaxAge > -1) {
                oss << "; Max-Age=" << this->MaxAge;
            }

            if (this->Secure) {
                oss << "; Secure";
            }

            if (this->HttpOnly) {
                oss << "; HttpOnly";
            }

            return oss.str();
        }
    };

    /**
     * Take a vector of Cookie structs and serialize them into HTTP cookie strings
     */
    std::string SerializeCookies(std::vector<Cookie>& cookieVec)
    {
        std::ostringstream oss;
        std::string serialized;
        bool first = true;

        // input has no cookies
        if (not cookieVec.size()) {
            return "";
        }

        // for ever cookie in vec
        for (Cookie& cookie : cookieVec) {
            // serialize it
            serialized = cookie.Serialize();
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
    std::vector<Cookie> DeserializeCookies(const std::string& cookieStr)
    {
        enum {
            C_NAME, C_VALUE, C_SAMESITE, C_DOMAIN, C_PATH, C_EXPIRES, C_MAXAGE
        } state = C_NAME;
        std::vector<Cookie> cookie_vec;
        std::istringstream iss(cookieStr);
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
            
            // consume one char from the http cookie string
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

                    // custom case for cookie attributes with parameters
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
