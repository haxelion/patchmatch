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

void Zone::draw(QPainter *painter, QImage *source, bool draw_outline)
{
    painter->save();
    if(draw_outline)
    {
        if(type == REPLACEZONE)
            painter->setPen(QColor(255, 255, 0));
        else
            painter->setPen(QColor(255, 0, 0));
        painter->drawRect(dst_x, dst_y, src_width, src_height);
    }
    if(type == FIXEDZONE)
    {
        painter->drawImage(dst_x, dst_y, *source, src_x, src_y, src_width, src_height);
    }
    painter->restore();
}

MaskedZone::MaskedZone(int src_x, int src_y, ZONETYPE type)
 : Zone(src_x, src_y, type)
{
    path = new QPainterPath();
    path->moveTo(src_x, src_y);
    dst_x = 0;
    dst_y = 0;
}

MaskedZone::~MaskedZone()
{
    delete path;
}

void MaskedZone::extend(int x, int y)
{
    path->lineTo(x, y);
}

void MaskedZone::finalize()
{
    path->closeSubpath();
}

bool MaskedZone::contains(int x, int y, int scale)
{
    return path->contains(QPoint(x*scale - dst_x, y*scale - dst_y));
}

void MaskedZone::draw(QPainter *painter, QImage *source, bool draw_outline)
{
    painter->save();
    painter->setTransform(QTransform().translate(dst_x, dst_y), true);
    if(draw_outline)
    {
        if(type == REPLACEZONE)
            painter->setPen(QColor(255, 255, 0));
        else
            painter->setPen(QColor(255, 0, 0));
        painter->drawPath(*path);
    }
    if(type == FIXEDZONE)
    {
        painter->setClipPath(*path);
        painter->drawImage(QPoint(0, 0), *source);
    }
    painter->restore();
}
