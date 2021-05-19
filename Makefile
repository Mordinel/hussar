CC    	= clang++
CFLAGS	= -std=c++20 -pthread -O3

SOURCES	= src/*.cpp
HEADERS = src/*.h
EXE     = hussar

.PHONY: run clean

$(EXE): $(SOURCES) $(HEADERS)
	$(CC) $(SOURCES) $(CFLAGS) -o $(EXE)

run: $(EXE)
	./$(EXE)

clean:
	rm -f $(EXE)

