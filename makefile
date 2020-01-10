CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -ansi -O3 -fPIC -fvisibility=hidden
LIB=-lm

SLIB=libcasino.so
TEST=test
TTT=tictactoe
BREAK=break
AMAZONS=amazons

src=$(shell find src/ -type f -name '*.c')
src_test=$(src) tests/test.c
src_tictactoe=$(src) games/tictactoe.c
src_breakthrough=$(src) games/breakthrough.c
src_amazons=$(src) games/amazons.c

obj_casino=$(src:.c=.o)
obj_test=$(src_test:.c=.o)
obj_tictactoe=$(src_tictactoe:.c=.o)
obj_breakthrough=$(src_breakthrough:.c=.o)
obj_amazons=$(src_amazons:.c=.o)

$(SLIB): $(obj_casino)
	$(CC) $(CFLAGS) -shared $^ -o $@ $(LIB)

$(TEST): $(obj_test)
	$(CC) $(CFLAGS) $^ -o $@ $(LIB)

$(TTT): $(obj_tictactoe)
	$(CC) $(CFLAGS) $^ -o $@ $(LIB)

$(BREAK): $(obj_breakthrough)
	$(CC) $(CFLAGS) $^ -o $@ $(LIB)

$(AMAZONS): $(obj_amazons)
	$(CC) $(CFLAGS) $^ -o $@ $(LIB)

.PHONY: clean
clean:
	rm -f $(obj_casino) $(obj_test) $(obj_tictactoe) $(obj_breakthrough) $(obj_amazons) $(SLIB) $(TEST) $(TTT) $(BREAK) $(AMAZONS)
