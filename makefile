APP=test
CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -ansi -O3
LIB=-lm

src=$(shell find src/ -type f -name '*.c')
src_test=$(src)
src_test+=tests/test.c
src_tictactoe=$(src)
src_tictactoe+=games/tictactoe.c
obj_test=$(src_test:.c=.o)
obj_tictactoe=$(src_tictactoe:.c=.o)

test: $(obj_test)
	$(CC) $(CFLAGS) $^ -o $(APP) $(LIB)

tictactoe: $(obj_tictactoe)
	$(CC) $(CFLAGS) $^ -o $(APP) $(LIB)

.PHONY: clean
clean:
	rm -f $(obj_test) $(obj_tictactoe) $(APP)
