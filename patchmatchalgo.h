#ifndef PATCHMATCHALGO_H
#define PATCHMATCHALGO_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <vector>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "zone.h"

class PatchMatchAlgo
{
public:
    bool done;
    bool terminate;
    float total_work;
    float work_done;
    int em_iteration;
    int patchmatch_iteration;
    int patch_w;
    GThread *thread;
    cairo_surface_t *source;
    cairo_surface_t *target;
    cairo_surface_t *reconstructed;
    std::vector<Zone> *zones;

    PatchMatchAlgo();
    void run(cairo_surface_t *source, cairo_surface_t *target, std::vector<Zone> *zones);
    float getProgress()
    {
        if(work_done != 0)
            return work_done / total_work; 
        else
            return 0;
    }
    bool isDone() { return done; }
    cairo_surface_t* getResult() { return reconstructed; }
    static gpointer threadFunction(gpointer data);
    void stop();
};

inline int distance(cairo_surface_t *source_scaled, cairo_surface_t *target, int sx, int sy, int tx, int ty, int patch_w);
inline cairo_surface_t * scaleSurface(cairo_surface_t *surface, int scale);
inline void randomANN(cairo_surface_t *source, cairo_surface_t *target, int **annx, int **anny, int **annd, int patch_w);
inline void rescaleANN(cairo_surface_t *source, cairo_surface_t *target, int **annx, int **anny, int **annd, int patch_w);
inline void patchVoting(cairo_surface_t *source_scaled, cairo_surface_t *source, cairo_surface_t *target, std::vector<Zone> *zones, int **annx, int **anny, int patch_w, int scale);
inline void patchMatch(cairo_surface_t *source, cairo_surface_t *target, std::vector<Zone> *zones, int **annx, int **anny, int **annd, int patch_w, int scale, int iter);

#endif
