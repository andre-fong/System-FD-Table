CC = gcc
CFLAGS = -Wall -Werror

a.out: a2.o
	$(CC) $(CFLAGS) -o $@ a2.o

a2.0: a2.c
	$(CC) $(CFLAGS) -c a2.c

.PHONY: clean
clean:
	rm *.o
	rm compositeTable.txt
	rm compositeTable.bin
