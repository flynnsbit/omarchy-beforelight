CC = gcc
PROTO = wlr-layer-shell-unstable-v1
CFLAGS = -fPIC -O2 -Wall -Wextra -I. $(shell pkg-config --cflags wayland-client sdl3)
LIBS = $(shell pkg-config --libs wayland-client sdl3) -ldl

.PHONY: all clean
all: libbeforelight-overlay.so

$(PROTO)-client-protocol.h: $(PROTO).xml
	wayland-scanner client-header $< $@

$(PROTO)-protocol.c: $(PROTO).xml
	wayland-scanner private-code $< $@

xdg-shell-client-protocol.h: xdg-shell.xml
	wayland-scanner client-header $< $@

xdg-shell-protocol.c: xdg-shell.xml
	wayland-scanner private-code $< $@

overlay.o: overlay.c $(PROTO)-client-protocol.h
	$(CC) $(CFLAGS) -c overlay.c -o $@

$(PROTO)-protocol.o: $(PROTO)-protocol.c $(PROTO)-client-protocol.h xdg-shell-client-protocol.h
	$(CC) $(CFLAGS) -c $(PROTO)-protocol.c -o $@

xdg-shell-protocol.o: xdg-shell-protocol.c xdg-shell-client-protocol.h
	$(CC) $(CFLAGS) -c xdg-shell-protocol.c -o $@

libbeforelight-overlay.so: overlay.o $(PROTO)-protocol.o xdg-shell-protocol.o
	$(CC) -shared -o $@ overlay.o $(PROTO)-protocol.o xdg-shell-protocol.o $(LIBS)

clean:
	rm -f *.o *.so $(PROTO)-client-protocol.h $(PROTO)-protocol.c \
		xdg-shell-client-protocol.h xdg-shell-protocol.c
