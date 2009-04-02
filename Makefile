CFLAGS=-ansi -pedantic -Wall -Wextra -D_XOPEN_SOURCE -D_BSD_SOURCE -O3
mv: mv.o

.PHONY: clean
clean:
	$(RM) mv *.o
