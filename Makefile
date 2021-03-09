CC    	=	clang++
CFLAGS	=	-std=c++20 -g -pthread

SOURCES	=	src/*.cpp
OUTFILE	=	hussar

.PHONY: run clean

hussar: $(SOURCES)
	$(CC) $(SOURCES) $(CFLAGS) -o $(OUTFILE)

run: $(OUTFILE)
	./$(OUTFILE)

clean:
	rm -f $(OUTFILE)

