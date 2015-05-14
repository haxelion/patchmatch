#include "patchmatchapp.h"
#include "patchmatchalgo.h"

PatchMatchAlgo::PatchMatchAlgo(PatchMatchApp *parent)
{
    em_iteration = 8;
    patchmatch_iteration = 5;
    thread = NULL;
    source = NULL;
    target = NULL;
    reconstructed = NULL;
    zones = NULL;
    this->parent = parent;
}

void PatchMatchAlgo::run(GdkPixbuf *source, GdkPixbuf *target, std::vector<Zone> *zones)
{
    if(thread != NULL)
        return;
    if(this->source != NULL)
        g_object_unref(this->source);
    if(this->target != NULL)
        g_object_unref(this->target);
    if(this->zones != NULL)
        delete this->zones;
    this->source = gdk_pixbuf_copy(source);
    this->target = gdk_pixbuf_copy(target);
    this->zones = new std::vector<Zone>(*zones);
    thread = g_thread_new("patchmatch", PatchMatchAlgo::threadFunction, this);
}

gpointer PatchMatchAlgo::threadFunction(gpointer data)
{
    PatchMatchAlgo *self = (PatchMatchAlgo*) data;
    GdkPixbuf *source_scaled = NULL;
    GdkPixbuf *target_scaled = NULL;
    // Compute starting scale
    int scale = 1 << (int)(log2(min(gdk_pixbuf_get_width(self->target), gdk_pixbuf_get_height(self->target))/self->patch_w)-1);
    // Compute total work to be done (pixel count)
    self->total_work = 0;
    self->work_done = 0;
    self->done = false;
    for(int i = scale; i >= 1; i = i/2)
    {
        self->total_work += self->em_iteration * gdk_pixbuf_get_width(self->target)/i * gdk_pixbuf_get_height(self->target)/i;
    }
    bool first_loop = true;
    while(scale >= 1)
    {
        if(source_scaled != NULL)
            g_object_unref(source_scaled);
        if(target_scaled != NULL)
            g_object_unref(target_scaled);
        source_scaled = gdk_pixbuf_scale_simple(
            self->source,
            gdk_pixbuf_get_width(self->target)/scale,
            gdk_pixbuf_get_height(self->target)/scale,
            GDK_INTERP_BILINEAR);
        target_scaled = gdk_pixbuf_scale_simple(
            self->source,
            gdk_pixbuf_get_width(self->target)/scale,
            gdk_pixbuf_get_height(self->target)/scale,
            GDK_INTERP_BILINEAR);
        if(first_loop)
        {
            first_loop = false;
        }
        for(int i = 0; i < self->em_iteration; i++)
        {
            // Updating work done and signaling to main thread
            self->work_done += gdk_pixbuf_get_width(self->target)/scale * gdk_pixbuf_get_height(self->target)/scale;
            g_idle_add(PatchMatchApp::cb_patchmatch_update, self->parent);
        }
        // Atomic replacement of reconstructed pixbuf
        GdkPixbuf *new_pixbuf = gdk_pixbuf_copy(target_scaled);
        GdkPixbuf *old_pixbuf = self->reconstructed;
        self->reconstructed = new_pixbuf;
        g_object_unref(old_pixbuf);
        // Signaling update to main thread
        g_idle_add(PatchMatchApp::cb_patchmatch_update, self->parent);
        scale = scale >> 1;
    }
    self->done = true;
    g_object_unref(source_scaled);
    g_object_unref(target_scaled);
    g_thread_unref(self->thread);
    self->thread = NULL;
    return NULL;
}

