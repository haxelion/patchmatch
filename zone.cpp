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

void Zone::draw(cairo_t *cr, cairo_surface_t *source, int scale, bool draw_outline)
{
    cairo_save(cr);
    cairo_scale(cr, 1.0/scale, 1.0/scale);
    cairo_rectangle(cr, dst_x, dst_y, src_width, src_height);
    if(draw_outline)
    {
        cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
        cairo_set_line_width(cr, scale*3.0);
        cairo_stroke_preserve(cr);
    }
    cairo_set_source_surface(cr, source, dst_x - src_x, dst_y - src_y);
    cairo_clip(cr);
    cairo_paint(cr);
    cairo_restore(cr);
}

Zone Zone::scale(int scale)
{
    Zone zs(src_x/scale, src_y/scale, type);
    zs.dst_x = dst_x/scale;
    zs.dst_y = dst_y/scale;
    zs.src_width = (src_x + src_width + scale - 1)/scale - zs.src_x;
    zs.src_height = (src_y + src_height + scale - 1)/scale - zs.src_y;
    return zs;
}

MaskedZone::MaskedZone(int src_x, int src_y, ZONETYPE type)
 : Zone(src_x, src_y, type)
{
    edges.push_back(std::make_pair(src_x, src_y));
}

void MaskedZone::extend(int x, int y)
{
    if(edges.back().first != x && edges.back().second != y)
        edges.push_back(std::make_pair(x, y));
}

void MaskedZone::finalize()
{
    src_x = edges[0].first;
    src_y = edges[0].second;
    src_width = edges[0].first;
    src_height = edges[0].second;
    for(unsigned int i = 0; i < edges.size(); i++)
    {
        if(edges[i].first < src_x)
            src_x = edges[i].first;
        if(edges[i].first > src_width)
            src_width = edges[i].first;
        if(edges[i].second < src_y)
            src_y = edges[i].second;
        if(edges[i].second > src_height)
            src_height = edges[i].second;
    }
    src_width -= src_x;
    src_height -= src_y;
    dst_x = src_x;
    dst_y = src_y;

}

bool MaskedZone::contains(int x, int y)
{
    return true;
}

Zone MaskedZone::scale(int scale)
{
    return Zone(src_x, dst_x, type);
}

void MaskedZone::draw(cairo_t *cr, cairo_surface_t *source, int scale, bool draw_outline)
{
}
