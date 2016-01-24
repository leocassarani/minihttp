bin/http: *.c
	mkdir -p bin
	gcc -std=gnu99 -Wall $? -o bin/http

clean:
	rm -f bin/http

.PHONY: clean
