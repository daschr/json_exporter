all:
	gcc -Wall -pedantic -O3 -o json-exporter json-exporter.c -ljansson -lprom -lpromhttp -lmicrohttpd -latomic

install:
	install json-exporter /usr/local/bin/

