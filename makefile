CC=gcc-10 -g -fsanitize=address -Wall

all: macD uds_c 

uds_s:
	gcc-10 -Wall uds_s.c -o uds_s

uds_c:	uds_c.c
	gcc-10 -Wall uds_c.c -o uds_c

macD:	macD.c
	$(CC) macD.c -o macD

clean:
	rm -f uds_s uds_c macD
