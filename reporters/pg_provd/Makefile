PREFIX ?= /usr  

#CFLAGS ?= -Wall -Werror 
CFLAGS ?= -Wall -g
CFLAGS += -std=gnu99

BINARIES = pg_cat  pg_provd pg_cleardb

all: $(BINARIES)

OBJECTS = plog pg_handlers pg_cat pg_provd pg_cleardb

clean:
	$(RM) $(BINARIES) $(addsuffix .o,$(OBJECTS))

pg_cat: pg_handlers.o plog.o pg_cat.o
pg_cat: LDLIBS += -lpq

pg_provd: pg_handlers.o plog.o pg_provd.o
pg_provd: LDLIBS += -lpq

pg_cleardb: pg_handlers.o pg_cleardb.o
pg_cleardb: LDLIBS += -lpq

.PHONY: all clean install
