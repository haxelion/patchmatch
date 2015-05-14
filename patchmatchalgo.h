#ifndef PATCHMATCHALGO_H
#define PATCHMATCHALGO_H

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <vector>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "zone.h"

class PatchMatchApp;

class PatchMatchAlgo
{
public:
    bool done;
    float total_work;
    float work_done;
    int em_iteration;
    int patchmatch_iteration;
    int patch_w;
    GThread *thread;
    GdkPixbuf *source;
    GdkPixbuf *target;
    GdkPixbuf *reconstructed;
    std::vector<Zone> *zones;
    PatchMatchApp *parent;

    PatchMatchAlgo(PatchMatchApp *parent);
    void run(GdkPixbuf *source, GdkPixbuf *target, std::vector<Zone> *zones);
    float getProgress()
    {
        if(work_done != 0)
            return work_done / total_work; 
        else
            return 0;
    }
    bool isDone() { return done; }
    GdkPixbuf* getResult() { return reconstructed; }
    static gpointer threadFunction(gpointer data);

    
};

#endif
