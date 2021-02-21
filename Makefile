CC=gcc

all: idle-monitor proc-events radeon-monitor xwindows-monitor xwindow-WM_DELETE_WINDOW

clean:
	rm idle-monitor proc-events radeon-monitor xwindows-monitor xwindow-WM_DELETE_WINDOW

idle-monitor: idle-monitor.c
	$(CC) -o $@ $< -lX11 -lXext `pkg-config --cflags --libs glib-2.0 gdk-3.0`

proc-events: proc-events.c
	$(CC) -o $@ $<

radeon-monitor: radeon-monitor.c
	$(CC) -o $@ $< `pkg-config --cflags --libs libdrm`

xwindows-monitor: xwindows-monitor.c
	$(CC) -o $@ $< `pkg-config --cflags --libs glib-2.0` -lX11

xwindow-WM_DELETE_WINDOW: xwindow-WM_DELETE_WINDOW.c
	$(CC) -o $@ $< -lX11

