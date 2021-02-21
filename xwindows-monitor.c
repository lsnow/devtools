/*
 * gcc windows.c -o windows `pkg-config --cflags --libs glib-2.0` -lX11
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <glib.h>

static Display *display = NULL;
static Window root = None;
static Atom atom_NET_ACTIVE_WINDOW;
static Atom atom_NET_CLIENT_LIST;

typedef struct
{
    GSource base;
    //GPollFD event_poll_fd;
    Display *display;
}XEventSource;

static void on_property_notify(XPropertyEvent *event);
static void on_create_notify(Window window);

static gboolean xevent_prepare(GSource *source, gint *timeout)
{
    *timeout = -1;
    XEventSource *xsource = (XEventSource *)source;
    Display *display = xsource->display;
    int n = XPending(display);
    return n;
}

static gboolean xevent_check(GSource *source)
{
    XEventSource *xsource = (XEventSource *)source;
    Display *display = xsource->display;
    int n = XPending(display);
    return n;
}

static gboolean xevent_dispatch(GSource *source, GSourceFunc callback, gpointer userdata)
{
    XEventSource *xsource = (XEventSource *)source;
    Display *display = xsource->display;
    while(XPending(display))
    {
        XEvent event;
        XNextEvent(display, &event);
        //XGetEventData(display, &event.xcookie);
        switch(event.type)
        {
            case ConfigureNotify:
                fprintf(stdout, "ConfigureNotify\n");
                break;
            case PropertyNotify:
                on_property_notify(&event.xproperty);
                break;
            case CreateNotify:
                on_create_notify(event.xcreatewindow.window);
            default:
                //fprintf(stdout, "%d\n", event.type);
                break;
        }
        //XFreeEventData(display, &event.xcookie);
    }
    return TRUE;
}

static GSourceFuncs xevent_funcs =
{
    xevent_prepare,
    xevent_check,
    xevent_dispatch
};

static unsigned char *get_property(Display *display,
                                   Window window,
                                   Atom atom,
                                   Atom reg_type,
                                   unsigned long *nitems_ret)
{
    long long_offset = 0;
    long long_length = LONG_MAX;
    Bool _delete = False;
    Atom actual_type_return;
    int actual_format_return;
    unsigned long bytes_after_return;
    unsigned char *prop_return = NULL;

    int result = XGetWindowProperty(display, window, atom, 
                                    long_offset, long_length, _delete,
                                    reg_type,
                                    &actual_type_return,
                                    &actual_format_return,
                                    nitems_ret,
                                    &bytes_after_return,
                                    &prop_return);
    if(result == Success)
        return prop_return;
    return NULL;
}

static void window_list_changed(void)
{
    unsigned long count = 0;
    Window *windows = (Window *)get_property(display,
                                             root,
                                             atom_NET_CLIENT_LIST,
                                             XA_WINDOW,
                                             &count);
    GDateTime *time = g_date_time_new_now_local ();
    for(int i = 0; i < (int)count; i++)
    {
        Window xwindow = windows[i];
        XClassHint hint;
        XGetClassHint(display, xwindow, &hint);
        fprintf(stdout, "%s: window list:%s %s\n", g_date_time_format (time, "%T"), hint.res_name, hint.res_class);
    }
    fprintf(stdout, "\n");
}

static void active_window_changed(void)
{
    unsigned long n = 0;
    Window *xid = (Window *)get_property(display, root,
                                         atom_NET_ACTIVE_WINDOW,
                                         XA_WINDOW, &n);
    Window window = *xid;
    XFree (xid);

    GDateTime *time = g_date_time_new_now_local ();
    XClassHint hint;
    XGetClassHint(display, window, &hint);
    fprintf(stdout, "%s: active window: %s %s\n", g_date_time_format (time, "%T"), hint.res_name, hint.res_class);
}

static void on_create_notify(Window window)
{
    GDateTime *time = g_date_time_new_now_local ();
    XClassHint hint;
    XGetClassHint(display, window, &hint);
    fprintf(stdout, "%s: create window: %s %s\n", g_date_time_format (time, "%T"), hint.res_name, hint.res_class);
}

static void on_property_notify(XPropertyEvent *event)
{
    Window window = event->window;
    Atom atom = event->atom;

    if(window == root)
    {
        if(atom == atom_NET_CLIENT_LIST)
            window_list_changed();
        else if(atom == atom_NET_ACTIVE_WINDOW)
        {
            active_window_changed();
        }
    }
}

int main ()
{
    display = XOpenDisplay(NULL);    
    root = DefaultRootWindow(display);
    atom_NET_CLIENT_LIST = XInternAtom(display, "_NET_CLIENT_LIST", False);
    atom_NET_ACTIVE_WINDOW = XInternAtom(display, "_NET_ACTIVE_WINDOW", False);

    XSelectInput(display, root, PropertyChangeMask | StructureNotifyMask | SubstructureNotifyMask);

    GSource *source = g_source_new(&xevent_funcs, sizeof(XEventSource));
    XEventSource *xsource = (XEventSource *)source;
    xsource->display = display;
    int fd = ConnectionNumber(display);
    g_source_add_unix_fd(source, fd, G_IO_IN);
    g_source_attach(source, NULL);

    window_list_changed();
    active_window_changed();

    GMainLoop *loop = g_main_loop_new (NULL, False);
    g_main_loop_run (loop);

    return 0;
}
