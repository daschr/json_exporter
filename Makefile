all:
	gcc -O3 -o json-exporter json-exporter.c -ljansson -lprom -lpromhttp -lmicrohttpd

install:
	install json-exporter /usr/local/bin/

