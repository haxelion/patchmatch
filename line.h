#ifndef LINE_H
#define LINE_H

#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <vector>
#include <utility>
#include <gtk/gtk.h>

#define min(a,b) (a < b ? a : b)
#define max(a,b) (a > b ? a : b)

#define RANSAC_ITER 100000

class Line
{
public:
    int x1, y1, x2, y2;

    Line(int x, int y);
    void extend(int x, int y);
    std::vector<std::pair<int, int> > getIntersect(int width, int height, int patch_w, int scale);
    void draw(cairo_t *cr, int scale);
    void applyLineConstraint(int **annx, int **anny, int width, int height, int patch_w, int scale, double threshold);
};

std::vector<std::pair<int, int> > getMatchedPatch(int **annx, int **anny, std::vector<std::pair<int, int> > patches);
Line ransac(std::vector<std::pair<int, int> > points, int patch_w, double threshold);

#endif
