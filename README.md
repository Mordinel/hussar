# hussar
A threaded web server

The default configuration can be changed in [main.cpp](./src/main.cpp)

## building

    git clone https://github.com/SOROM2/hussar.git --recurse-submodules
    cd hussar
    make
   
## running

    ./hussar -h
    Usage: ./hussar [-hv -i <ipv4> -p <port> -t <thread count> -d <document root>]
            -h              Display this help
            -v              Verbose console output
            -i <IPV4>       Ipv4 to bind to
            -p <PORT>       Port to listen on
            -t <THREAD>     Threads to use
            -d <DIR>        Document root directory
