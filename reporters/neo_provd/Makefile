PREFIX ?= /usr  

#CFLAGS ?= -Wall -Werror 
CFLAGS ?= -Wall -g
CFLAGS += -std=gnu99
#LDFLAGS+=-Wl,-rpath -Wl,/usr/local/lib
BINARIES = neo_cat neo_provd 


OBJECTS = plog curl_handlers neo_handlers neo_cat neo_provd

all: $(BINARIES)

clean:
	$(RM) $(BINARIES) $(addsuffix .o,$(OBJECTS)) $(OBJECTS) *~ *.pyc

neo_cat: curl_handlers.o neo_handlers.o plog.o neo_cat.o
neo_cat: LDLIBS+= -lcurl -lpq -ljansson

neo_provd: curl_handlers.o neo_handlers.o plog.o neo_provd.o
neo_provd: LDLIBS += -lcurl -lpq -ljansson

.PHONY: all clean install
