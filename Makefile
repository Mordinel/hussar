# source files
SOURCES      := src/*.cpp
HEADERS      := src/*.h
PCH_SOURCE   := src/pch.h

# compiler flags
CC           := clang++
CFLAGS       := -std=c++20 -Wall -O2 -pthread -I ./include/
CFLAGS_DEBUG := -std=c++20 -Wall -Og -pthread -I ./include/ -g -D DEBUG

# executable
EXE          := hussar
EXE_ARGS     := -d docroot -v

.PHONY: release debug run clean


# make cli options
release: $(PCH_SOURCE).pch $(EXE)

debug: $(PCH_SOURCE)-debug.pch $(EXE)-debug

run:
	if [ -f $(EXE)-debug ]; then lldb -o run ./$(EXE)-debug $(EXE_ARGS); elif [ -f $(EXE) ]; then ./$(EXE) $(EXE_ARGS); fi

clean:
	rm -f $(EXE) $(PCH_SOURCE).pch $(EXE)-debug $(PCH_SOURCE)-debug.pch

# release
$(PCH_SOURCE).pch: $(PCH_SOURCE)
	if [ -f $(PCH_SOURCE)-debug.pch ]; then rm -f $(PCH_SOURCE)-debug.pch; fi
	$(CC) $(CFLAGS) -x c++-header $(PCH_SOURCE) -o $(PCH_SOURCE).pch

$(EXE): $(SOURCES) $(HEADERS)
	if [ -f $(EXE)-debug ]; then rm -f $(EXE)-debug; fi
	$(CC) $(CFLAGS) -include-pch $(PCH_SOURCE).pch $(SOURCES) -o $(EXE)

# debug
$(PCH_SOURCE)-debug.pch: $(PCH_SOURCE)
	if [ -f $(PCH_SOURCE).pch ]; then rm -f $(PCH_SOURCE).pch; fi
	$(CC) $(CFLAGS_DEBUG) -x c++-header $(PCH_SOURCE) -o $(PCH_SOURCE)-debug.pch

$(EXE)-debug: $(SOURCES) $(HEADERS)
	if [ -f $(EXE) ]; then rm -f $(EXE); fi
	$(CC) $(CFLAGS_DEBUG) -include-pch $(PCH_SOURCE)-debug.pch $(SOURCES) -o $(EXE)-debug
