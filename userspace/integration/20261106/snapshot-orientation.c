#define _GNU_SOURCE
#include <gst/gst.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Scoped to the cepheus Snapshot launcher. Keep the CPU copy workaround and
 * adjust camera tags, leaving raw sensor pixels and CFA order unchanged. */
typedef struct {
    int base, pose, last_pose;
    gboolean have_base, mirror, injecting;
    gint64 checked;
} CameraOrientation;

static int read_pose(CameraOrientation *s)
{
    gint64 now = g_get_monotonic_time();
    if (now - s->checked < 100000)
        return s->pose;
    s->checked = now;
    const char *path = getenv("CEPHEUS_CAMERA_POSE_FILE");
    FILE *f = fopen(path ? path : "/run/cepheus-camera/orientation", "r");
    char value[32];
    if (f) {
        if (fscanf(f, "%31s", value) == 1) {
            if (!strcmp(value, "normal")) s->pose = 0;
            else if (!strcmp(value, "left-up")) s->pose = 90;
            else if (!strcmp(value, "bottom-up")) s->pose = 180;
            else if (!strcmp(value, "right-up")) s->pose = 270;
        }
        fclose(f);
    }
    return s->pose;
}

static gchar *orientation_tag(CameraOrientation *s, int pose)
{
    /* Horizontal mirroring reverses rotation composition. */
    int degrees = (s->base + (s->mirror ? -pose : pose) + 360) % 360;
    return g_strdup_printf("%srotate-%d", s->mirror ? "flip-" : "", degrees);
}

static GstPadProbeReturn orient_probe(GstPad *pad, GstPadProbeInfo *info, gpointer data)
{
    CameraOrientation *s = data;
    if (s->injecting)
        return GST_PAD_PROBE_OK;
    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
        if (GST_EVENT_TYPE(event) == GST_EVENT_TAG) {
            GstTagList *tags;
            gchar *original = NULL;
            gst_event_parse_tag(event, &tags);
            if (gst_tag_list_get_string(tags, GST_TAG_IMAGE_ORIENTATION, &original)) {
                const char *number = original;
                s->mirror = g_str_has_prefix(number, "flip-");
                if (s->mirror) number += 5;
                int angle;
                if (sscanf(number, "rotate-%d", &angle) == 1 &&
                    (angle == 0 || angle == 90 || angle == 180 || angle == 270)) {
                    /* Confirmed upright on hardware: the original preview was inverted. */
                    s->base = (angle + 180) % 360;
                    s->have_base = TRUE;
                    s->last_pose = read_pose(s);
                    gchar *corrected = orientation_tag(s, s->last_pose);
                    GstTagList *copy = gst_tag_list_copy(tags);
                    gst_tag_list_add(copy, GST_TAG_MERGE_REPLACE, GST_TAG_IMAGE_ORIENTATION, corrected, NULL);
                    GstEvent *replacement = gst_event_new_tag(copy);
                    gst_event_set_seqnum(replacement, gst_event_get_seqnum(event));
                    gst_event_unref(event);
                    GST_PAD_PROBE_INFO_DATA(info) = replacement;
                    g_message("cepheus camera orientation: %s -> %s (pose=%d)", original, corrected, s->last_pose);
                    g_free(corrected);
                }
                g_free(original);
            }
        }
    } else if ((GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) && s->have_base) {
        int pose = read_pose(s);
        if (pose != s->last_pose) {
            gchar *value = orientation_tag(s, pose);
            GstEvent *event = gst_event_new_tag(gst_tag_list_new(GST_TAG_IMAGE_ORIENTATION, value, NULL));
            s->injecting = TRUE;
            gst_pad_push_event(pad, event);
            s->injecting = FALSE;
            s->last_pose = pose;
            g_message("cepheus camera auto-rotation: %s (pose=%d)", value, pose);
            g_free(value);
        }
    }
    return GST_PAD_PROBE_OK;
}

static void add_capture_rotation(GObject *object, const char *property)
{
    if (!g_object_class_find_property(G_OBJECT_GET_CLASS(object), property)) return;
    GstElement *existing = NULL;
    g_object_get(object, property, &existing, NULL);
    if (existing) { gst_object_unref(existing); return; }
    GstElement *flip = gst_element_factory_make("videoflip", NULL);
    if (flip) {
        gst_object_ref_sink(flip);
        gst_util_set_object_arg(G_OBJECT(flip), "video-direction", "auto");
        g_object_set(object, property, flip, NULL);
        gst_object_unref(flip);
        g_message("cepheus camera: enabled automatic %s rotation", property);
    }
}

GObject *g_object_new_with_properties(GType type, guint n,
                                    const char *names[], const GValue values[])
{
    typedef GObject *(*NewFn)(GType, guint, const char **, const GValue *);
    NewFn next = (NewFn)dlsym(RTLD_NEXT, "g_object_new_with_properties");
    if (!next) abort();
    GObject *object = next(type, n, names, values);
    if (!object) return object;
    if (!strcmp(g_type_name(type), "GstPipeWireSrc")) {
        g_object_set(object, "always-copy", TRUE, NULL);
        GstPad *pad = gst_element_get_static_pad(GST_ELEMENT(object), "src");
        if (pad) {
            CameraOrientation *s = g_new0(CameraOrientation, 1);
            s->last_pose = -1;
            gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM | GST_PAD_PROBE_TYPE_BUFFER,
                              orient_probe, s, g_free);
            gst_object_unref(pad);
        }
    } else if (!strcmp(g_type_name(type), "GstCameraBin")) {
        add_capture_rotation(object, "image-filter");
        add_capture_rotation(object, "video-filter");
    }
    return object;
}
