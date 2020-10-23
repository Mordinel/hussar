CFLAGS=-std=c++20 -g
SOURCES=src/*.cpp
OUTFILE=hussarHTTP

.PHONY: clean

all: ${SOURCES}
	clang++ ${SOURCES} ${CFLAGS} -o ${OUTFILE}

run: ${OUTFILE}
	./${OUTFILE}

clean:
	rm -f ${OUTFILE}

