PREFIX?=/usr/X11R6
CFLAGS?=-Os -pedantic -Wall

all:
	$(CC) $(CFLAGS) -I$(PREFIX)/include llwm.c -L$(PREFIX)/lib -lX11 -o llwm

clean:
	rm -f llwm
