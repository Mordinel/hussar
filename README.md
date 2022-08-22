# hussar
A threaded web server

The default configuration can be changed in [main.cpp](./src/main.cpp)

[Here](./examples/auth_upload.cpp) is an example of usage in your own code

## building the file web server

    git clone https://github.com/SOROM2/hussar.git --recurse-submodules
    cd hussar
    make

## running as a file web server

    ./hussar -h
    Usage: ./hussar [-hv -i <ipv4> -p <port> -t <thread count> -d <document root> -k <ssl private key> -c <ssl certificate>]
            -h              Display this help
            -v              Verbose console output
            -i <IPV4>       Ipv4 to bind to
            -p <PORT>       Port to listen on
            -t <THREAD>     Threads to use
            -d <DIR>        Document root directory
            -k <key.pem>    SSL Private key
            -c <cert.pem>   SSL Certificate

## building the example

    git clone https://github.com/SOROM2/hussar.git --recurse-submodules
    cd hussar
    make example
    make certs

## running the example

    ./hello_world

