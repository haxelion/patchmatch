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

Zone::~Zone()
{
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
    dst_x = src_x;
    dst_y = src_y;
    src_width = abs(src_width);
    src_height = abs(src_height);
}

bool Zone::contains(int x, int y, int scale)
{
    if(scale*x + scale - 1 >= dst_x && scale*x <= dst_x + src_width &&
       scale*y + scale - 1 >= dst_y && scale*y <= dst_y + src_height)
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
        if(type == REPLACEZONE)
        {
            cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
        }
        else
            cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
        cairo_set_line_width(cr, scale*3.0);
        cairo_stroke_preserve(cr);
    }
    if(type == FIXEDZONE)
    {
        cairo_set_source_surface(cr, source, dst_x - src_x, dst_y - src_y);
        cairo_clip(cr);
        cairo_paint(cr);
    }
    cairo_restore(cr);
}

MaskedZone::MaskedZone(int src_x, int src_y, ZONETYPE type)
 : Zone(src_x, src_y, type)
{
    edges.push_back(std::make_pair(src_x, src_y));
}

MaskedZone::~MaskedZone()
{
    cairo_surface_destroy(mask);
}

void MaskedZone::extend(int x, int y)
{
    if(edges.back().first != x || edges.back().second != y)
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
    mask = cairo_image_surface_create(CAIRO_FORMAT_A8, src_width, src_height);
    cairo_t *cr = cairo_create(mask);
    cairo_translate(cr, -src_x, -src_y);
    cairo_move_to(cr, edges[0].first, edges[0].second);
    for(unsigned int i = 1; i < edges.size(); i++)
        cairo_line_to(cr, edges[i].first, edges[i].second);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    cairo_fill(cr);
    cairo_surface_flush(mask);
    cairo_destroy(cr);
}

bool MaskedZone::contains(int x, int y, int scale)
{
    if(scale*x + scale - 1 >= dst_x && scale*x <= dst_x + src_width &&
       scale*y + scale - 1 >= dst_y && scale*y <= dst_y + src_height)
    {
        unsigned char *p = cairo_image_surface_get_data(mask);
        int stride = cairo_image_surface_get_stride(mask);
        p += scale*(y - dst_y)*stride + x - dst_x;
        for(int i = 0; i < scale; i++)
        {
            for(int j = 0; j < scale; j++)
            {
                if(*p > 0)
                    return true;
                p++;
            }
            p += stride - scale;
        }
    }
    return false;
}

void MaskedZone::draw(cairo_t *cr, cairo_surface_t *source, int scale, bool draw_outline)
{
    cairo_save(cr);
    cairo_scale(cr, 1.0/scale, 1.0/scale);
    cairo_translate(cr, dst_x - src_x, dst_y - src_y);
    cairo_move_to(cr, edges[0].first, edges[0].second);
    for(unsigned int i = 1; i < edges.size(); i++)
        cairo_line_to(cr, edges[i].first, edges[i].second);
    cairo_close_path(cr);
    if(draw_outline)
    {
        if(type == REPLACEZONE)
            cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
        else
            cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
        cairo_set_line_width(cr, scale*3.0);
        cairo_stroke_preserve(cr);
    }
    if(type == FIXEDZONE)
    {
        cairo_set_source_surface(cr, source, 0, 0);
        cairo_clip(cr);
        cairo_paint(cr);
    }
    cairo_restore(cr);
}
