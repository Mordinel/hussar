#ifndef HUSSAR_PCH_H
#define HUSSAR_PCH_H

// c includes
#include <cstdlib>      // exit
#include <netdb.h>      // getnameinfo
#include <unistd.h>     // close
#include <string.h>     // memset
#include <signal.h>     // sigs
#include <arpa/inet.h>  // htons
#include <sys/types.h>  // compatibility reasons
#include <netinet/in.h>
#include <sys/socket.h> // sockets

// cpp includes
#include <filesystem>       // file reading
#include <fstream>          // file streams
#include <thread>           // threading
#include <regex>            // path stuff

#include "util.h"                    // utilities
#include "thread_pool/thread_pool.h" // thread management

#endif
