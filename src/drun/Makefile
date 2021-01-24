target := drun
objs   := drun.o

CC     := gcc
CFLAGS := -O3 -std=c99 -pedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wno-unused-result
INC    := -I ../../include/
LIBS   := -lcurl
PREFIX := /usr/local

# Compile the program
all: $(target)
$(target): $(objs)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $<

# Phony targets
.PHONY: install uninstall clean
install: $(target)
	mkdir -p $(PREFIX)/bin
	cp $(target) $(PREFIX)/bin/$(target)

uninstall:
	rm -f $(PREFIX)/bin/$(target)

clean:
	rm -f $(target) $(objs)
