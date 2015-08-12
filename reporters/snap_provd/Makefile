#
#	Makefile for this SNAP example
#	- modify Makefile.ex when creating a new SNAP example
#
#	implements:
#		all (default), clean
#

include ../../Makefile.config
include Makefile.ex


all: $(MAIN)

# COMPILE
snap_cat: snap_cat.cpp plog.cpp plog.hpp snap_handlers.cpp snap_handlers.hpp $(EXSNAP)/Snap.o 
	g++ -g -std=c++98 -Wall -O3 -DNDEBUG -o snap_cat snap_cat.cpp plog.cpp snap_handlers.cpp ../../snap-core/Snap.o -I../../snap-core -I../../snap-adv -I../../glib-core -I../../snap-exp  -lrt

snap_provd: snap_provd.cpp plog.cpp plog.hpp snap_handlers.cpp snap_handlers.hpp $(EXSNAP)/Snap.o 
	g++ -g -std=c++98 -Wall -O3 -DNDEBUG -o snap_provd snap_provd.cpp plog.cpp snap_handlers.cpp ../../snap-core/Snap.o -I../../snap-core -I../../snap-adv -I../../glib-core -I../../snap-exp  -lrt

$(EXSNAP)/Snap.o: 
	make -C $(EXSNAP)

clean:
	rm -f *.o  $(MAIN)  $(MAIN).exe
	rm -rf Debug Release

#g++ -std=c++98 -Wall -O3 -DNDEBUG -o snap_cat snap_cat.cpp  plog.cpp ../../snap-core/Snap.o -I../../snap-core -I../../snap-adv -I../../glib-core -I../../snap-exp  -lrt

#include ../Makefile.exmain