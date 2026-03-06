CC = gcc
CFLAGS = -Wall -O2 -g -I.
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)
X11_LIBS = -lX11

all: blackline-wm blackline-panel blackline-launcher blackline-tools blackline-background blackline-fm blackline-editor

blackline-wm: wm/wm.c
	$(CC) $(CFLAGS) -o $@ $< $(X11_LIBS)

blackline-panel: panel/panel.c tools/minimized_container.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ panel/panel.c tools/minimized_container.c $(GTK_LIBS) $(X11_LIBS)

blackline-launcher: launcher/launcher.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $< $(GTK_LIBS)

blackline-tools: tools/tools_container.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ $< $(GTK_LIBS)

blackline-background: tools/background.c
	$(CC) $(CFLAGS) -o $@ $<

blackline-fm: tools/file-manager/fm.c tools/file-manager/browser.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ tools/file-manager/fm.c tools/file-manager/browser.c $(GTK_LIBS)

# Text editor with edit features
blackline-editor: tools/text_editor/editor.c tools/text_editor/edit.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -o $@ tools/text_editor/editor.c tools/text_editor/edit.c $(GTK_LIBS)

# Individual object files
%.o: %.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c $< -o $@

clean:
	rm -f blackline-wm blackline-panel blackline-launcher blackline-tools blackline-background blackline-fm blackline-editor *.o

# Install all binaries
install: all
	sudo cp blackline-wm /usr/local/bin/
	sudo cp blackline-panel /usr/local/bin/
	sudo cp blackline-launcher /usr/local/bin/
	sudo cp blackline-tools /usr/local/bin/
	sudo cp blackline-background /usr/local/bin/
	sudo cp blackline-fm /usr/local/bin/
	sudo cp blackline-editor /usr/local/bin/

# Uninstall all binaries
uninstall:
	sudo rm -f /usr/local/bin/blackline-wm
	sudo rm -f /usr/local/bin/blackline-panel
	sudo rm -f /usr/local/bin/blackline-launcher
	sudo rm -f /usr/local/bin/blackline-tools
	sudo rm -f /usr/local/bin/blackline-background
	sudo rm -f /usr/local/bin/blackline-fm
	sudo rm -f /usr/local/bin/blackline-editor

# Run the editor directly
run-editor: blackline-editor
	./blackline-editor


run-wm: blackline-wm
	./blackline-wm

.PHONY: all clean install uninstall run-editor run-wm