CC=gcc -std=gnu99 -Wall -g `pkg-config --cflags glib-2.0`

OBJS = getopt.o main.o serial.o
uterm: $(OBJS)
	$(CC) -o $@ $(OBJS) `pkg-config --libs glib-2.0`

%.o: %.c
	$(CC) -c $<

install: uterm
	cp uterm /usr/bin/

clean:
	rm -rf *.o *~ uterm
