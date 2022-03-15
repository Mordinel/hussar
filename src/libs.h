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

// c includes
#include <cstdlib>          // exit
#include <netdb.h>          // getnameinfo
#include <unistd.h>         // close
#include <string.h>         // memset
#include <signal.h>         // sigs
#include <arpa/inet.h>      // htons
#include <sys/types.h>      // compatibility reasons
#include <netinet/in.h>
#include <sys/socket.h>     // sockets
#include <openssl/ssl.h>    // openssl
#include <openssl/err.h>
#include <openssl/rand.h>   // csprng

// cpp includes
#include <filesystem>       // file reading
#include <functional>       // functional programming
#include <fstream>          // file streams
#include <thread>           // threading
#include <regex>            // path stuff

#include "util.h"                    // utilities
#include "thread_pool/thread_pool.h" // thread management

