# source files
SOURCES      := src/*.cpp
HEADERS      := src/*.h

# compiler flags
CXXFLAGS       := -std=c++20 -Wall -O2 -pthread -lcrypto -lssl -I ./include/

# executable
EXE          := hussar
EXE_ARGS     := -d docroot -v

.PHONY: release run clean example certs

# make cli options
release:
	$(CXX) $(SOURCES) $(CXXFLAGS) -o $(EXE)

run:
	./$(EXE) $(EXE_ARGS)

clean:
	rm -f $(EXE) hello_world

example:
	$(CXX) -I ./src/ ./examples/hello_world.cpp $(CXXFLAGS) -o hello_world

certs:
	openssl req -newkey rsa:2048 -nodes -keyout key.pem -x509 -days 9 -out cert.pem
