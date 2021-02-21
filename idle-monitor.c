#include <stdlib.h>
#include <stdio.h>

#include <glib.h>
#include <X11/Xlib.h>
#include <X11/extensions/sync.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>

typedef struct _IdleMonitor
{
    Display *dpy;
    GdkDisplay *display;
    guint idle_time;
    gint sync_event;
    XSyncCounter idle_counter;
    XSyncAlarm xalarm;
}IdleMonitor;

static GdkFilterReturn
xevent_filter(GdkXEvent *gdkxevent, GdkEvent *event, gpointer data)
{
    XEvent *xev = (XEvent *)gdkxevent;
    IdleMonitor *monitor = (IdleMonitor *)data;
    if (xev->type != monitor->sync_event + XSyncAlarmNotify)
        return GDK_FILTER_CONTINUE;

    printf("idle %ds\n", monitor->idle_time);
    //return handle_alarm_notify_event(monitor, xev);
}

int main()
{
    int sync_error;
    int ncounters;
    XSyncSystemCounter *counters;
    gint major, minor;
    gint i;

    IdleMonitor *monitor = g_new0(IdleMonitor, 1);
    monitor->idle_time = 1000 * 10; // 10 senconds
    //monitor->display = gdk_display_get_default();
    monitor->display = gdk_display_open(NULL);
    monitor->dpy = GDK_DISPLAY_XDISPLAY(monitor->display);

    if (!XSyncQueryExtension(monitor->dpy,
                             &monitor->sync_event,
                             &sync_error))
    {
        g_warning ("X have no Xync extension");
        return 0;
    }
    if (!XSyncInitialize(monitor->dpy, &major, &minor))
    {
        return 0;
    }
    counters = XSyncListSystemCounters (monitor->dpy, &ncounters);
    for (i = 0; i < ncounters; i++)
    {
        if (counters[i].name != NULL
                && strcmp (counters[i].name, "IDLETIME") == 0)
        {
            monitor->idle_counter = counters[i].counter;
            break;
        }
    }
    XSyncFreeSystemCounterList (counters);

    if (!monitor->idle_counter) {
        g_warning ("No idle counter");
        return 0;
    }

    XSyncAlarmAttributes attr;
    XSyncValue delta;
    unsigned int flags;
    XSyncTestType test;

    /* which way do we do the test? */
    //if (alarm_type == GPM_IDLETIME_ALARM_TYPE_POSITIVE)
        test = XSyncPositiveTransition;
    //else
    //    test = XSyncNegativeTransition;

    XSyncIntToValue (&delta, 0);

    attr.trigger.counter = monitor->idle_counter;
    attr.trigger.value_type = XSyncAbsolute;
    attr.trigger.test_type = test;
    XSyncValue interval;
    XSyncIntToValue(&interval, monitor->idle_time);
    attr.trigger.wait_value = interval;
    attr.delta = delta;
    attr.events = True;

    flags = XSyncCACounter |
            XSyncCAValueType |
            XSyncCATestType |
            XSyncCAValue |
            XSyncCADelta |
            XSyncCAEvents;

    monitor->xalarm = XSyncCreateAlarm (monitor->dpy,
                                        flags,
                                        &attr);
    gdk_window_add_filter (NULL,
                           xevent_filter,
                           monitor);

    GMainLoop *loop = g_main_loop_new (NULL, 0);
    g_main_loop_run(loop);
    return 0;
}
