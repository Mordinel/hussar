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

namespace hussar {
    struct Config {
        std::string host;
        std::string private_key;
        std::string certificate;
        uint16_t port;
        uint32_t thread_count;
        bool verbose;

        Config()
        {
            this->host = "127.0.0.1";
            this->private_key = "";
            this->certificate = "";
            this->port = 8080;
            this->thread_count = 0;
            this->verbose = false;
        }

        Config(Config&& config)
        {
            this->host = config.host;
            this->private_key = config.private_key;
            this->certificate = config.certificate;
            this->port = config.port;
            this->thread_count = config.thread_count;
            this->verbose = config.verbose;

        }

        Config& operator=(Config&& config)
        {
            this->host = config.host;
            this->private_key = config.private_key;
            this->certificate = config.certificate;
            this->port = config.port;
            this->thread_count = config.thread_count;
            this->verbose = config.verbose;
            return *this;
        }
    };
};

