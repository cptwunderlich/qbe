BIN = minic
CANARY = get_random_canary

CFLAGS += -g -Wall

$(BIN): yacc minic.y $(CANARY).s
	./yacc minic.y
	$(CC) $(CFLAGS) -o $@ y.tab.c

$(CANARY).s: $(CANARY).c
	$(CC) -g -S -fno-stack-protector -fpic $(CANARY).c

clean:
	rm -f yacc minic y.* $(CANARY).s

.PHONY: clean
