target := bin/drun
objs   := drun.o

CC     := gcc
CFLAGS := -O3 -std=c99 -pedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wno-unused-result
LIBS   := -lcurl
PREFIX := /usr/local

# Compile the program
all: $(target)
$(target): $(objs)
	@mkdir -p bin/
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $< $(DEPFLAGS)

# Phony targets
.PHONY: install uninstall clean
install: $(NAME)
	mkdir -p $(PREFIX)/bin
	cp $(target) $(PREFIX)/$(target)

uninstall:
	rm -f $(PREFIX)/$(target)

clean:
	rm -f $(target) $(objs) $(deps)
