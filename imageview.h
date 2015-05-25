#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QWidget>
#include <QSize>
#include <QImage>
#include <QPainter>
#include <QMouseEvent>
#include <vector>
#include <cmath>
#include "zone.h"

enum Tool
{
    TOOL_NONE,
    TOOL_MOVE,
    TOOL_DELETE,
    TOOL_RESHUFFLE_RECTANGLE,
    TOOL_RESHUFFLE_FREE_HAND,
    TOOL_REPLACE_RECTANGLE,
    TOOL_REPLACE_FREE_HAND
};

struct Move
{
    int zone;
    int dx, dy;
};

class ImageView : public QWidget
{
    Q_OBJECT
public:
    explicit ImageView(QWidget *parent = 0);
    ~ImageView();
    QSize sizeHint() const {return QSize(200, 200);}
    QSize minimumSizeHint() const {return QSize(200, 200);}
    void setImages(QImage *source, QImage *target);
    void setRetargetScales(double xscale, double yscale);
    void setZones(std::vector<Zone*> *zones){this->zones = zones;}
    void setActiveTool(Tool tool) {active_tool = tool; dragging = false;}
    void setWorking(bool working) {this->working = working;}

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);

private:
    QImage *source, *target;
    QPainter *painter;
    double scale, xscale, yscale;
    std::vector<Zone*> *zones;
    Tool active_tool;
    Move current_move;
    bool dragging;
    bool working;
};

#endif // IMAGEVIEW_H
