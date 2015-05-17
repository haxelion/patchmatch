#ifndef ZONE_H
#define ZONE_H

#include <stdlib.h>
#include <math.h>
#include <vector>
#include <utility>
#include <gtk/gtk.h>

#define min(a,b) (a < b ? a : b)
#define max(a,b) (a > b ? a : b)

enum ZONETYPE
{
    FIXEDZONE,
    REPLACEZONE
};

class Zone
{
public:
    int src_x, src_y, src_height, src_width;
    ZONETYPE type;
    int dst_x, dst_y;
 
    Zone(int src_x, int src_y, ZONETYPE type);
    void move(int dx, int dy);
    virtual void extend(int x, int y);
    virtual void finalize();
    virtual bool contains(int x, int y);
    virtual Zone scale(int scale);
    virtual void draw(cairo_t *cr, cairo_surface_t *source, int scale, bool draw_outline);
};

class MaskedZone : public Zone
{
public:
    std::vector<std::pair<int, int> > edges;

    MaskedZone(int src_x, int src_y, ZONETYPE type);
    void extend(int x, int y);
    void finalize();
    bool contains(int x, int y);
    Zone scale(int scale);
    void draw(cairo_t *cr, cairo_surface_t *source, int scale, bool draw_outline);
};

#endif
