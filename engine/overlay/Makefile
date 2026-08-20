CC = gcc
PROTO = wlr-layer-shell-unstable-v1
OUTDIR ?= .
CFLAGS = -fPIC -O2 -Wall -Wextra -I. -I$(OUTDIR) $(shell pkg-config --cflags wayland-client sdl3)
LIBS = $(shell pkg-config --libs wayland-client sdl3) -ldl

.PHONY: all clean
all: $(OUTDIR)/libbeforelight-overlay.so

$(OUTDIR):
	mkdir -p $(OUTDIR)

$(OUTDIR)/$(PROTO)-client-protocol.h: $(PROTO).xml | $(OUTDIR)
	wayland-scanner client-header $< $@

$(OUTDIR)/$(PROTO)-protocol.c: $(PROTO).xml | $(OUTDIR)
	wayland-scanner private-code $< $@

$(OUTDIR)/xdg-shell-client-protocol.h: xdg-shell.xml | $(OUTDIR)
	wayland-scanner client-header $< $@

$(OUTDIR)/xdg-shell-protocol.c: xdg-shell.xml | $(OUTDIR)
	wayland-scanner private-code $< $@

$(OUTDIR)/overlay.o: overlay.c $(OUTDIR)/$(PROTO)-client-protocol.h | $(OUTDIR)
	$(CC) $(CFLAGS) -c overlay.c -o $@

$(OUTDIR)/$(PROTO)-protocol.o: $(OUTDIR)/$(PROTO)-protocol.c $(OUTDIR)/$(PROTO)-client-protocol.h $(OUTDIR)/xdg-shell-client-protocol.h
	$(CC) $(CFLAGS) -c $(OUTDIR)/$(PROTO)-protocol.c -o $@

$(OUTDIR)/xdg-shell-protocol.o: $(OUTDIR)/xdg-shell-protocol.c $(OUTDIR)/xdg-shell-client-protocol.h
	$(CC) $(CFLAGS) -c $(OUTDIR)/xdg-shell-protocol.c -o $@

$(OUTDIR)/libbeforelight-overlay.so: $(OUTDIR)/overlay.o $(OUTDIR)/$(PROTO)-protocol.o $(OUTDIR)/xdg-shell-protocol.o
	$(CC) -shared -o $@ $^ $(LIBS)

clean:
	rm -f $(OUTDIR)/overlay.o $(OUTDIR)/*.so \
		$(OUTDIR)/$(PROTO)-client-protocol.h $(OUTDIR)/$(PROTO)-protocol.c \
		$(OUTDIR)/xdg-shell-client-protocol.h $(OUTDIR)/xdg-shell-protocol.c \
		$(OUTDIR)/$(PROTO)-protocol.o $(OUTDIR)/xdg-shell-protocol.o
