SRCS=$(wildcard *.c)
HEAD=$(wildcard *.h)

bin/http: $(SRCS) $(HEAD)
	mkdir -p bin
	gcc -std=gnu99 -Wall -pthread $(SRCS) -o bin/http

clean:
	rm -f bin/http

.PHONY: clean
