/*
 * SECTION:  xwindow.c
 * @Title: xwindow.c
 * @Short_Description:  
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <X11/X.h>
#include <X11/Xlib.h>

int main ()
{
    Display *dpy = XOpenDisplay(NULL);
    Window root = DefaultRootWindow (dpy);

    Window w = XCreateSimpleWindow (dpy, root, 0, 0, 640, 640, 0, 0, 0);

    XSelectInput (dpy, w,
                  ExposureMask | KeyPressMask | StructureNotifyMask);

    XEvent e;
    XMapWindow (dpy, w);
    XNextEvent (dpy, &e);

    Atom delete, ping;
    delete = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
    ping = XInternAtom (dpy, "WM_PING", False);

    Atom atoms[2];
    atoms[0] = delete;
    atoms[1] = ping;
    XSetWMProtocols (dpy, w, atoms, 2);

    while (1)
    {
        XNextEvent (dpy, &e);
        if (e.type == ClientMessage)
        {
            printf("ClientMessage\n");
            while (1)
                sleep (1);
        }
        else
        {
            sleep (1);
        }
    }
    return 0;
}
