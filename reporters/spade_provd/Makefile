PREFIX ?= /usr  

#CFLAGS ?= -Wall -Werror 
#CFLAGS ?= -Wall -g
CFLAGS ?= -g
CFLAGS += -std=gnu99
#LDFLAGS+=-Wl,-rpath -Wl,/usr/local/lib
BINARIES = spade_cat spade_provd 


OBJECTS = plog  spade_handlers spade_cat spade_provd

all: $(BINARIES)

clean:
	$(RM) $(BINARIES) $(addsuffix .o,$(OBJECTS)) $(OBJECTS) *~ *.pyc

spade_cat:  spade_handlers.o plog.o spade_cat.o
spade_cat: LDLIBS+= -lcurl 

spade_provd:  spade_handlers.o plog.o spade_provd.o
spade_provd: LDLIBS += -lcurl 

.PHONY: all clean install
