target := drun
objs   := drun.o

CC     := gcc
CFLAGS := -O3 -std=c99 -pedantic -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -Wno-unused-result
LIBS   := -lcurl
PREFIX := /usr/local

all: $(target)

# Automatic dependency tracking
deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

# Compile the program
$(target): $(objs)
	@mkdir -p bin/
	$(CC) $(CFLAGS) -o bin/$@ $^ $(LIBS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $< $(DEPFLAGS)

# Phony targets
.PHONY: install uninstall clean
install: $(NAME)
	mkdir -p $(PREFIX)/bin
	cp $(target) $(PREFIX)/bin/$(target)

uninstall:
	rm -f $(PREFIX)/bin/$(target)

clean:
	rm -f $(target) $(objs) $(deps)
