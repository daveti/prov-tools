PREFIX ?= /usr  

CFLAGS ?= -Wall -Werror 
CFLAGS += -std=gnu99 -static

BINARIES = pbang ncred elijah fixbang
OBJECTS = plog

all: $(BINARIES)

clean:
	$(RM) $(BINARIES) $(addsuffix .o,$(OBJECTS))

install: $(addprefix install-,$(BINARIES))

install-%: %
	install -D $< $(DESTDIR)$(PREFIX)/sbin/$<

ncred elijah: plog.o

pbang: LDLIBS += -lz -luuid

.PHONY: all clean install
