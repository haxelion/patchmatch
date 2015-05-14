#ifndef ZONE_H
#define ZONE_H

#include <stdlib.h>
#include <math.h>

#define min(a,b) a < b ? a : b
#define max(a,b) a > b ? a : b

enum ZONETYPE
{
    FIXEDZONE,
    REPLACEZONE
};

class Zone
{
public:
    int src_x, src_y, src_height, src_width;
    int **mask;
    ZONETYPE type;
    int dst_x, dst_y;
 
    Zone(int src_x, int src_y, ZONETYPE type);
    void move(int dx, int dy);
    void extend(int x, int y);
    void finalize();
    bool contains(int x, int y);

};

#endif
