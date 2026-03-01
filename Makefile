CC = gcc
CFLAGS = -Wall -O2 -g
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)

all: blackline-wm blackline-panel blackline-launcher blackline-tools blackline-background

blackline-wm: wm/wm.c
	$(CC) $(CFLAGS) -o $@ $< -lX11

blackline-panel: panel/panel.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $< $(GTK_LIBS)

blackline-launcher: launcher/launcher.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $< $(GTK_LIBS)

blackline-tools: tools/tools_container.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $< $(GTK_LIBS)

blackline-background: tools/background.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f blackline-*

run:
	./run.sh