#include "zone.h"

Zone::Zone(int src_x, int src_y, ZONETYPE type)
{
    this->src_x = src_x;
    this->src_y = src_y;
    this->src_height = 0;
    this->src_width = 0;
    this->dst_x = src_x;
    this->dst_y = src_y;
    this->type = type;
}

void Zone::move(int dx, int dy)
{
    dst_x += dx;
    dst_y += dy;
}

void Zone::extend(int x, int y)
{
    src_width = x - src_x; 
    src_height = y - src_y; 
}

void Zone::finalize()
{
    src_x = min(src_x, src_x + src_width);
    src_y = min(src_y, src_y + src_height);
    src_width = abs(src_width);
    src_height = abs(src_height);
}

bool Zone::contains(int x, int y)
{
    if(x >= dst_x && x <= dst_x + src_width &&
       y >= dst_y && y <= dst_y + src_height)
        return true;
    return false;
}

Zone Zone::scale(int scale)
{
    Zone zs(src_x/scale, src_y/scale, type);
    zs.dst_x = dst_x/scale;
    zs.dst_y = dst_y/scale;
    zs.src_height = src_height/scale;
    zs.src_width = src_width/scale;
    return zs;
}
