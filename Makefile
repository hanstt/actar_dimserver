DIM_PATH:=/LynxOS/mbsusr/mbsdaq/dim_v20r20
CPPFLAGS:=-I$(DIM_PATH)/dim
CFLAGS:=-ansi -pedantic-errors -Wall -Werror -Wextra -Wshadow
LIBS:=$(DIM_PATH)/linux/libdim.a -pthread

all: build/actar build/compass

build/actar: build/actar.o
	g++ -o $@ $< $(LIBS)

build/compass: build/compass.o
	g++ -o $@ $< $(LIBS)

build/%.o: %.cpp Makefile
	[ -d $(@D) ] || mkdir -p $(@D)
	g++ -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

clean:
	rm -rf build
