# source files
SOURCES      := src/*.cpp
HEADERS      := src/*.h

# compiler flags
CXXFLAGS       := -std=c++20 -Wall -O2 -pthread -lcrypto -lssl -I ./include/

OPENSSL_INCLUDE_PATH := $(wildcard /usr/local/Cellar/openssl@3*/*/include)
ifneq ("$(OPENSSL_INCLUDE_PATH)","")
CXXFLAGS += -I$(OPENSSL_INCLUDE_PATH)
endif

OPENSSL_LIB_PATH := $(wildcard /usr/local/Cellar/openssl@3*/*/lib)
ifneq ("$(OPENSSL_LIB_PATH)","")
LDFLAGS += -L$(OPENSSL_LIB_PATH)
endif

# executable
EXE          := hussar
EXE_ARGS     := -d docroot -v

.PHONY: release run clean example certs

# make cli options
release:
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(SOURCES) -o $(EXE)

run:
	./$(EXE) $(EXE_ARGS)

clean:
	rm -f $(EXE) hello_world

example:
	$(CXX) $(CXXFLAGS) -I ./src/ ./examples/hello_world.cpp -o hello_world

certs:
	openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 9 -out cert.pem
