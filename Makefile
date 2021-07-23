CC    	= clang++
CFLAGS	= -std=c++20 -pthread -Og -g -I ./include/

SOURCES	= src/*.cpp
HEADERS = src/*.h
EXE     = hussar

.PHONY: run clean

$(EXE): $(SOURCES) $(HEADERS)
	$(CC) $(SOURCES) $(CFLAGS) -o $(EXE)

run: $(EXE)
	./$(EXE) -d docroot -v

clean:
	rm -f $(EXE)

