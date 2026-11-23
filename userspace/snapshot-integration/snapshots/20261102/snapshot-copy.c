#define _GNU_SOURCE
#include <glib-object.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>
/* Scope this compatibility shim to Snapshot. GTK's dmabuf import path leaks
 * duplicated descriptors on this stack; pipewiresrc's copy mode supplies
 * ordinary CPU memory while preserving caps and video transform metadata. */
GObject *g_object_new_with_properties(GType type, guint n,
                                    const char *names[], const GValue values[])
{
    typedef GObject *(*NewFn)(GType, guint, const char **, const GValue *);
    NewFn next = (NewFn)dlsym(RTLD_NEXT, "g_object_new_with_properties");
    if (!next) abort();
    GObject *object = next(type, n, names, values);
    if (object && !strcmp(g_type_name(type), "GstPipeWireSrc")) {
        g_object_set(object, "always-copy", TRUE, NULL);
        g_message("cepheus: enabled CPU buffer copy for PipeWire camera source");
    }
    return object;
}
