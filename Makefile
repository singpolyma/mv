CFLAGS=-ansi -pedantic -Wall -Wextra -O3

mv: mv.o getopt.o

mv.o: mv.c
	$(CC) $(CFLAGS) -D_XOPEN_SOURCE -D_BSD_SOURCE -c $^

.PHONY: clean
clean:
	$(RM) mv *.o
