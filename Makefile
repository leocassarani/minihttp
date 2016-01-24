bin/http: *.c *.h
	mkdir -p bin
	gcc -std=gnu99 -Wall http.c main.c server.c -o bin/http

clean:
	rm -f bin/http

.PHONY: clean
