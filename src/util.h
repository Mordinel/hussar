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

#include <unordered_map>
#include <string_view>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>

#define SERVER_NAME "hussar"

#define hus hussar

namespace hussar {
    std::mutex print_mtx;
    std::unique_lock<std::mutex> print_lock(print_mtx);
    std::unordered_map<std::string, std::string> mimes = {
        {"woff", "font/woff"},
        {"woff2", "font/woff2"},
        {"js", "application/javascript"},
        {"ttf", "application/octet-stream"},
        {"otf", "application/octet-stream"},
        {"exe", "application/octet-stream"},
        {"pdf", "application/pdf"},
        {"zip", "application/zip"},
        {"ico", "image/x-icon"},
        {"gif", "image/gif"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"css", "text/css"},
        {"html", "text/html"},
        {"txt", "text/plain"}
    };


    /**
     * performs url decoding on str
     */
    std::string url_decode(const std::string& str)
    {
        char c;
        std::ostringstream oss;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '%') {
                i++;
                std::string code{str.substr(i, 2)};
                c = static_cast<char>(std::strtol(code.c_str(), NULL, 16));
                if (c) oss << c;
                i++;
            } else if (str[i] == '+') {
                oss << ' ';
            } else {
                oss << str[i];
            }
        }
        return oss.str();
    }
    
    /**
     * performs html escaping on str
     */
    std::string html_escape(const std::string& str)
    {
        std::ostringstream oss;
        for (size_t i = 0; i < str.size(); ++i) {
            switch (str[i]) {
                default:
                    oss << str[i];
                    break;
                case '&':
                    oss << "&amp;";
                    break;
                case '<':
                    oss << "&lt;";
                    break;
                case '>':
                    oss << "&gt;";
                    break;
                case '"':
                    oss << "&#34;";
                    break;
                case '\'':
                    oss << "&#39;";
                    break;
                case ';':
                    oss << "&#59;";
                    break;
            }
        }

        return oss.str();
    }

    /**
     * strips terminal control chars from a string
     */
    std::string strip_terminal_chars(const std::string& str)
    {
        std::ostringstream oss;
    
        for (char c : str) {
            switch (c) {
                case 0x07:
                case 0x08:
                case 0x09:
                case 0x0a:
                case 0x0b:
                case 0x0c:
                case 0x0d:
                case 0x1b:
                case 0x7f:
                    break;
                default:
                    oss << c;
                    break;
            }
        }
    
        return oss.str();
    }

    /**
     * Trims whitespace from the start and end of a string
     */
    std::string trim(const std::string& str)
    {
        std::ostringstream oss;
        size_t content_start = 0;
        size_t content_end = 0;

        // get content start and end
        for (size_t n = 0; n < str.size(); ++n) {
            switch (str[n]) {
                default:
                    content_end = n;
                    break;
                case ' ':
                case '\t':
                    if (content_end) content_start = n + 1;
                    break;
            }
        }

        // format string with only content
        for (size_t n = 0; n < str.size(); ++n) {
            if (n <= content_end && n >= content_start) {
                oss << str[n];
            }
        }
        
        return oss.str();
    }

    /**
     * returns a string containing the mime time of the extension of the document string.
     */
    std::string get_mime(std::filesystem::path& document) {
        std::string extension = document.extension();
        if (extension.size() < 2) {
            return "application/octet-stream";
        }

        auto mimeIter = mimes.find(extension.substr(1));
        if (mimeIter == mimes.end()) {
            return "application/octet-stream";
        }

        return mimeIter->second;
    }

    /**
     * splits the str into dest delimited by char c
     */
    template <typename T>
    std::vector<T> split_string(std::string_view str, const char c)
    {
        std::string::size_type i = 0;
        std::string::size_type j = str.find(c);
        std::vector<T> dest;
    
        while (j != std::string::npos) {
            dest.emplace_back(str.substr(i, j - i));
            i = ++j;
            j = str.find(c, j);
            if (j == std::string::npos) {
                dest.emplace_back(str.substr(i, str.length()));
            }
        }

        return dest;
    }

    /**
     * splits the str into dest delimited by char c
     */
    template <typename T>
    std::vector<T> split_string(std::string_view str, const std::string& delim)
    {
        std::string::size_type i = 0;
        std::string::size_type j = str.find(delim);
        std::vector<T> dest;
    
        while (j != std::string::npos) {
            dest.emplace_back(str.substr(i, j - i));
            j += delim.size();
            i = j;
            j = str.find(delim, j);
            if (j == std::string::npos) {
                dest.emplace_back(str.substr(i, str.length()));
            }
        }

        return dest;
    }

    /**
     * joins the strings into one string delimited by char c
     */
    template <typename T>
    std::string join_string(std::vector<T>& strings, const char c)
    {
        if (strings.size() == 0) return "";
        if (strings.size() == 1) return std::string{strings[0]};

        std::ostringstream oss{std::string{strings[0]}};
        for (size_t n = 1; n < strings.size(); ++n) {
            oss << c << strings[n];
        }

        return oss.str();
    }

    /**
     * joins the strings into one string delimited by string delim
     */
    template <typename T>
    std::string join_string(std::vector<T>& strings, const std::string& delim)
    {
        if (strings.size() == 0) return "";
        if (strings.size() == 1) return std::string{strings[0]};

        std::ostringstream oss{std::string{strings[0]}};
        for (size_t n = 1; n < strings.size(); ++n) {
            oss << delim << strings[n];
        }

        return oss.str();
    }

    /**
     * for fatal errors that should kill the program.
     */
    void fatal_error(const std::string& message)
    {
        print_lock.lock();
            std::cerr << message << std::endl;
        print_lock.unlock();
        std::exit(1);
    }

    /**
     * returns true if the name is alphabetic + '_', else returns false
     */
    bool validate_param_name(const std::string& name)
    {
        if (not name.size())
            return false;

        char c;
        for (size_t n = 0; n < name.size(); ++n) {
            c = name[n];

            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
                continue;
            else
                return false;

        }
        return true;
    }
}

