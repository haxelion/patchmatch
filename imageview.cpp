#include "imageview.h"

ImageView::ImageView(QWidget *parent) : QWidget(parent)
{
    painter = new QPainter();
    source = NULL;
    target = NULL;
    zones = NULL;
    dragging = false;
    working = false;
    xscale = 1.0;
    yscale = 1.0;
}

ImageView::~ImageView()
{
    delete painter;
}

void ImageView::paintEvent(QPaintEvent *event)
{
    painter->begin(this);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    if(target != NULL)
    {
        if(working)
            scale = fmin(height()/(double)target->height(), width()/(double)target->width());
        else
            scale = fmin(height()/(double)target->height()/yscale, width()/(double)target->width()/xscale);
        painter->scale(scale, scale);
        painter->save();
        if(working == false)
            painter->scale(xscale, yscale);
        painter->drawImage(QPoint(0, 0), *target);
        painter->restore();
        if(working == false)
            for(unsigned int i = 0; i < zones->size(); i++)
                zones->at(i)->draw(painter, source, true);
    }
    painter->end();
}

void ImageView::mouseMoveEvent(QMouseEvent * event)
{
    if(dragging)
    {
        if(!(event->buttons() & Qt::LeftButton))
            dragging = false;
        else if(active_tool == TOOL_MOVE)
        {
            zones->at(current_move.zone)->dst_x = event->x()/scale + current_move.dx;
            zones->at(current_move.zone)->dst_y = event->y()/scale + current_move.dy;
            update();
        }
        else if((active_tool == TOOL_RESHUFFLE_RECTANGLE ||
            active_tool == TOOL_RESHUFFLE_FREE_HAND ||
            active_tool == TOOL_REPLACE_RECTANGLE ||
            active_tool == TOOL_REPLACE_FREE_HAND) &&
            zones->size() >= 0)
        {
            zones->back()->extend(event->x()/scale, event->y()/scale);
            update();
        }
    }
}

void ImageView::mousePressEvent(QMouseEvent * event)
{
    if(event->button() == Qt::LeftButton)
    {
        if(active_tool == TOOL_DELETE)
        {
            for(int i = zones->size() - 1; i >= 0; i--)
            {
                if(zones->at(i)->contains(event->x()/scale, event->y()/scale, 1))
                {
                    delete zones->at(i);
                    zones->erase(zones->begin() + i);
                    update();
                }
            }
        }
        else if(active_tool == TOOL_MOVE)
        {
            for(int i = zones->size() - 1; i >= 0; i--)
            {
                if(zones->at(i)->contains(event->x()/scale, event->y()/scale, 1))
                {
                    dragging = true;
                    current_move.zone = i;
                    current_move.dx = zones->at(i)->dst_x - event->x()/scale;
                    current_move.dy = zones->at(i)->dst_y - event->y()/scale;
                    break;
                }
            }
        }
        else if(active_tool == TOOL_RESHUFFLE_RECTANGLE)
        {
            dragging = true;
            zones->push_back(new Zone(event->x()/scale, event->y()/scale, FIXEDZONE));
            update();
        }
        else if(active_tool == TOOL_RESHUFFLE_FREE_HAND)
        {
            dragging = true;
            zones->push_back(new MaskedZone(event->x()/scale, event->y()/scale, FIXEDZONE));
            update();
        }
        else if(active_tool == TOOL_REPLACE_RECTANGLE)
        {
            dragging = true;
            zones->push_back(new Zone(event->x()/scale, event->y()/scale, REPLACEZONE));
            update();
        }
        else if(active_tool == TOOL_REPLACE_FREE_HAND)
        {
            dragging = true;
            zones->push_back(new MaskedZone(event->x()/scale, event->y()/scale, REPLACEZONE));
            update();
        }
    }
}

void ImageView::mouseReleaseEvent(QMouseEvent * event)
{
    if(event->button() == Qt::LeftButton && dragging == true)
    {
        dragging = false;
        if((active_tool == TOOL_RESHUFFLE_RECTANGLE ||
            active_tool == TOOL_RESHUFFLE_FREE_HAND ||
            active_tool == TOOL_REPLACE_RECTANGLE ||
            active_tool == TOOL_REPLACE_FREE_HAND) &&
            zones->size() >= 0)
        {
            zones->back()->finalize();
            update();
        }
    }
}
