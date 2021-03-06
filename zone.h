#ifndef ZONE_H
#define ZONE_H

#include <stdlib.h>
#include <math.h>
#include <vector>
#include <utility>
#include <QPainter>
#include <QPainterPath>

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
    virtual ~Zone();
    void move(int dx, int dy);
    virtual void extend(int x, int y);
    virtual void finalize();
    virtual bool contains(int x, int y, int scale);
    virtual void draw(QPainter *painter, QImage *source, bool draw_outline);
};

class MaskedZone : public Zone
{
public:
    std::vector<std::pair<int, int> > edges;
    QPainterPath *path;

    MaskedZone(int src_x, int src_y, ZONETYPE type);
    ~MaskedZone();
    void extend(int x, int y);
    void finalize();
    bool contains(int x, int y, int scale);
    void draw(QPainter *painter, QImage *source, bool draw_outline);
};

#endif
