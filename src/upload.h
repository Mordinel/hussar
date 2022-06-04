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

#include <string>
#include "util.h"

namespace hussar {

    struct UploadedFile {
        std::string id;
        std::string name;
        std::string mime;
        std::string data;
        bool valid = false;

        UploadedFile(const std::string& boundary, const std::string& body)
        {
            if (boundary.size() == 0) return;
            if (body.size() == 0) return;

            size_t start = body.find(boundary);
            if (start == std::string::npos) return;
            start += boundary.size();
            start += 2; // for the \r\n

            size_t end = body.find(boundary, start);

            std::string file_component = body.substr(start, end - start - 4);

            std::vector<std::string_view> file_components = split_string<std::string_view>(file_component, "\r\n");

            size_t n;
            for (n = 0; n < file_components.size(); ++n) {
                auto& line = file_components[n];
                if (line == "") {
                    ++n; // start of the file content
                    break;
                }
                if (line.find("Content-Disposition: ") != std::string_view::npos) {
                    std::string content_disposition = extract_header_content(line);
                    parse_content_disposition(content_disposition);
                } else if (line.find("Content-Type: ") != std::string_view::npos) {
                    this->mime = extract_header_content(line);
                }
            }
            std::ostringstream oss;
            oss << file_components[n++];
            for (; n < file_components.size(); ++n) {
                oss << "\r\n" << file_components[n];
            }

            this->data = oss.str();
            valid = true;
        }

        UploadedFile(const std::string& id, const std::string& name, const std::string& mime, const std::string& data)
            : id(id), name(name), mime(mime), data(data)
        {
            valid = true;
        }

        // moveable
        UploadedFile(UploadedFile&& file)
            : id(file.id), name(file.name), mime(file.mime), data(std::move(file.data))
        {}

        // not copyable
        UploadedFile(UploadedFile& file) = delete;
        UploadedFile(const UploadedFile& file) = delete;
        UploadedFile& operator=(UploadedFile& file) = delete;
        UploadedFile& operator=(const UploadedFile& file) = delete;

    private:
        void parse_content_disposition(std::string& content) {
            std::vector<std::string> d_components = split_string<std::string>(content, "; ");

            if (d_components[0] != "form-data") return;

            for (size_t n = 1; n < d_components.size(); ++n) {
                std::vector<std::string> attrib = split_string<std::string>(d_components[n], "=");
                if (attrib.size() != 2) break;

                std::string& key = attrib[0];
                std::string& value = attrib[1];

                if (key == "name") {
                    this->id = filter_name(value);
                } else if (key == "filename") {
                    this->name = filter_name(value);
                }
            }
        }
    };
};

