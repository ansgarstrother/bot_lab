CC=gcc -std=gnu99 -Wall -g `pkg-config --cflags glib-2.0`

OBJS = main.o getopt.o uorc.o vhash.o varray.o

all: uisp


uisp: $(OBJS)
	$(CC) -o $@ $(OBJS) `pkg-config --libs glib-2.0`

%.o: %.c
	$(CC) -c $<

install: uisp
	cp uisp /usr/bin/

clean:
	rm -rf *.o *~ uterm
