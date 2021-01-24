objs   := retime.o

ifdef WIN
	CC     := i686-w64-mingw32-gcc
	target := ../../bin/retime-windows.exe
else
	CC     := gcc
	target := ../../bin/retime
	LIBS   := -lm
endif
CFLAGS := -O3 -std=c99 -pedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wno-unused-result
INC    := -I ../../include/
PREFIX := /usr/local

# Compile the program
all: $(target)
$(target): $(objs)
	@mkdir -p ../../bin
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
