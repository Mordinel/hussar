#ifndef HUSSAR_UTIL_H
#define HUSSAR_UTIL_H

#include <unordered_map>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>

#define SERVER_NAME "hussar"

#define hus hussar

namespace hussar {
    std::mutex PrintMut;
    std::unique_lock<std::mutex> PrintLock(PrintMut);
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
    std::string UrlDecode(const std::string& str)
    {
        char c;
        std::ostringstream oss;
        for (size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '%') {
                i++;
                std::string code(str.substr(i, 2));
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
     * strips terminal control chars from a string
     */
    std::string StripString(const std::string& str)
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
     * returns a string containing the mime time of the extension of the document string.
     */
    std::string GetMime(std::string& document) {
        size_t l = document.find_last_of('.');
    
        // if no char found
        if (l == std::string::npos) {
            return mimes.begin()->second; // default to 1st item (text file)
        }
    
        // create a string of the extension
        std::string extension(document.begin() + l + 1, document.end());
    
        // find extension in mimes
        auto mimeIter = mimes.find(extension);
    
        if (mimeIter == mimes.end()) {
            return mimes.begin()->second;
        }

        return mimeIter->second;
    }

    
    /**
     * splits the string str into vector strVec, delimited by char c
     */
    void SplitString(const std::string& str, char c, std::vector<std::string>& strVec)
    {
        std::string::size_type i = 0;
        std::string::size_type j = str.find(c);
    
        while (j != std::string::npos) {
            strVec.push_back(str.substr(i, j - i));
            i = ++j;
            j = str.find(c, j);
            if (j == std::string::npos) {
                strVec.push_back(str.substr(i, str.length()));
            }
        }
    }

    /**
     * for fatal errors that should kill the program.
     */
    void Error(const std::string& message)
    {
        PrintLock.lock();
            std::cerr << message << std::endl;
        PrintLock.unlock();
        std::exit(1);
    }
 
}

#endif
