BIN = minic

CFLAGS += -g -Wall

$(BIN): yacc minic.y
	./yacc minic.y
	$(CC) $(CFLAGS) -o $@ y.tab.c
	$(CC) -g -S -fno-stack-protector -fpic get_random_canary.c

clean:
	rm -f yacc minic y.* get_random_canary.s

.PHONY: clean
