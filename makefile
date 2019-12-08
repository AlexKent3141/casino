APP=test
CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -ansi -g
LIB=-lm

src=$(shell find src/ -type f -name '*.c')
obj=$(src:.c=.o)

test: $(obj)
	$(CC) $(CFLAGS) $^ -o $(APP) $(LIB)

.PHONY: clean
clean:
	rm -f $(obj) $(APP)
