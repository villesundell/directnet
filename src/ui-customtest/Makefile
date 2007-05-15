CXX=g++
CXXFLAGS=-O2 -g
AR=ar
ARFLAGS=rc

OBJS=ui.o

all: libui.a

listlibs:
	@echo "-levent"

libui.a: $(OBJS)
	$(AR) $(ARFLAGS) libui.a $(OBJS)

.SUFFIXES: .cc .o

.cc.o:
	$(CXX) -I.. -I../.. $(CXXFLAGS) -c $< -o $@


clean:
	rm -f libui.a $(OBJS)

distclean: clean
