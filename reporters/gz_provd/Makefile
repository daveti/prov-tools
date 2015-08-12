CFLAGS ?= -Wall -Werror 
CFLAGS += -std=gnu99 -static

BINARIES = gz_cat gz_provd

# Add app ipc support for provd
PROVD_OBJS=gz_provd.o app_ipc.o

all: $(BINARIES)

clean:
	$(RM) $(BINARIES) $(addsuffix .o,$(OBJECTS))
	$(RM) -rf *.o 2>&1

gz_cat: plog.o

#gz_provd: LDLIBS += -lz
# app ipc support
LDLIBS += -lz
gz_provd: $(PROVD_OBJS)
	  $(CC) $(CFLAGS) $(PROVD_OBJS) $(LDLIBS) -o gz_provd

.PHONY: all clean install
