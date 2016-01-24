bin/http: *.c *.h
	mkdir -p bin
	gcc -std=gnu99 -Wall server.c main.c -o bin/http

clean:
	rm -f bin/http

.PHONY: clean
