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
    GdkPixbuf *source;
    GdkPixbuf *target;
    GdkPixbuf *reconstructed;
    std::vector<Zone> *zones;

    PatchMatchAlgo();
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
    void stop();
};

inline int distance(GdkPixbuf *source, GdkPixbuf *target, int sx, int sy, int tx, int ty, int patch_w);
inline void randomANN(GdkPixbuf *source, GdkPixbuf *target, int **annx, int **anny, int **annd, int patch_w);
inline void rescaleANN(GdkPixbuf *source, GdkPixbuf *target, int **annx, int **anny, int **annd, int patch_w);
inline void patchVoting(GdkPixbuf *source, GdkPixbuf *target, std::vector<Zone> *zones, int **annx, int **anny, int patch_w, int scale);
inline void patchMatch(GdkPixbuf *source, GdkPixbuf *target, std::vector<Zone> *zones, int **annx, int **anny, int **annd, int patch_w, int scale, int iter);

#endif
