#include "line.h"

Line::Line(int x, int y)
{
    x1 = x;
    x2 = x;
    y1 = y;
    y2 = y;
}

void Line::extend(int x, int y)
{
    x2 = x;
    y2 = y;
}

std::vector<std::pair<int, int> > Line::getIntersect(int width, int height, int patch_w, int scale)
{
    double x1 = this->x1/(double)scale;
    double y1 = this->y1/(double)scale;
    double x2 = this->x2/(double)scale;
    double y2 = this->y2/(double)scale;
    double dx = x2 - x1;
    double dy = y2 - y1;
    double x = x1;
    double y = y1;
    std::vector<std::pair<int, int > > patches;

    for(int i = 0; i < patch_w; i++)
        for(int j = 0; j < patch_w; j++)
            if(((int) x - i) >= 0 && ((int) y - j) >= 0 && 
               ((int) x - i) < width - patch_w && ((int) y - j) < height - patch_w)
                patches.push_back(std::make_pair((int) x - i, (int) y - j));

    while((x - x1)/dx <= 1 && (y - y1)/dy <= 1)
    {
        double lx = (round(x + dx/fabs(dx)/1.9999) - x)/dx;
        double ly = (round(y + dy/fabs(dy)/1.9999) - y)/dy;
        if(lx == ly)
        {
            x += dx*lx;
            y += dy*ly;
            int xa = dx > 0 ? 0 : patch_w - 1;
            int ya = dy > 0 ? 0 : patch_w - 1;
            for(int i = 0; i < patch_w; i++)
                if(((int) x - xa) >= 0 && ((int) y - i) >= 0 && 
                   ((int) x - xa) < width - patch_w && ((int) y - i) < height - patch_w)
                    patches.push_back(std::make_pair((int) x - xa, (int) y - i));
            for(int i = 0; i < patch_w; i++)
                if(i != xa)
                    if(((int) x - i) >= 0 && ((int) y - ya) >= 0 && 
                       ((int) x - i) < width - patch_w && ((int) y - ya) < height - patch_w)
                        patches.push_back(std::make_pair((int) x - i, (int) y - ya));
        }
        else if(lx < ly)
        {
            x += dx*lx;
            y += dy*lx;
            int xa = dx > 0 ? 0 : patch_w - 1;
            for(int i = 0; i < patch_w; i++)
                if(((int) x - xa) >= 0 && ((int) y - i) >= 0 && 
                   ((int) x - xa) < width - patch_w && ((int) y - i) < height - patch_w)
                    patches.push_back(std::make_pair((int) x - xa, (int) y - i));
        }
        else
        {
            x += dx*ly;
            y += dy*ly;
            int ya = dy > 0 ? 0 : patch_w - 1;
            for(int i = 0; i < patch_w; i++)
                if(((int) x - i) >= 0 && ((int) y - ya) >= 0 && 
                   ((int) x - i) < width - patch_w && ((int) y - ya) < height - patch_w)
                    patches.push_back(std::make_pair((int) x - i, (int) y - ya));
        }
    }
    return patches;
}

void Line::draw(cairo_t *cr, int scale)
{
    cairo_save(cr);
    cairo_scale(cr, 1.0/scale, 1.0/scale);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    cairo_set_line_width(cr, scale*3.0);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void Line::applyLineConstraint(int **annx, int **anny, int width, int height, int patch_w, int scale, double threshold)
{
    std::vector<std::pair<int, int> > patches = getIntersect(width, height, patch_w, scale);
    std::vector<std::pair<int, int> > matched = getMatchedPatch(annx, anny, patches);
    Line source_line = ransac(matched, patch_w, threshold/scale);
    printf("Target line: (%d, %d) -> (%d, %d)\n", x1, y1, x2, y2);
    printf("Source line: (%d, %d) -> (%d, %d)\n", source_line.x1, source_line.y1, source_line.x2, source_line.y2);
    double tdx = (x2 - x1)/(double) scale;
    double tdy = (y2 - y1)/(double) scale;
    double tdl = sqrt(tdx*tdx + tdy*tdy);
    double sdx = source_line.x2 - source_line.x1;
    double sdy = source_line.y2 - source_line.y1;
    double sdl = sqrt(sdx*sdx + sdy*sdy);
    printf("td = (%lf, %lf), sd = (%lf, %lf)\n", tdx, tdy, sdx, sdy);
    for(unsigned int i = 0; i < patches.size(); i++)
    {
        double td = ((patches[i].first + patch_w/2.0 - x1/(double) scale)*tdy - tdx*(patches[i].second + patch_w/2.0 - y1/(double) scale))/tdl;
        double sd = ((matched[i].first + patch_w/2.0 - source_line.x1)*sdy - sdx*(matched[i].second + patch_w/2.0 - source_line.y1))/sdl;
        if(fabs(sd) < threshold)
        {
            double nx = matched[i].first - sdy/sdl * (td - sd) - patch_w/2.0;
            double ny = matched[i].first + sdx/sdl * (td - sd) - patch_w/2.0;
            nx = min(max(0, nx), width - patch_w - 1);
            ny = min(max(0, ny), height - patch_w - 1);
            annx[patches[i].first][patches[i].second] = (int)round(nx);
            anny[patches[i].first][patches[i].second] = (int)round(ny);
        }
    }
}

std::vector<std::pair<int, int> > getMatchedPatch(int **annx, int **anny, std::vector<std::pair<int, int> > patches)
{
    std::vector<std::pair<int, int> > matched;
    for(unsigned int i = 0; i < patches.size(); i++)
        matched.push_back(std::make_pair(annx[patches[i].first][patches[i].second],anny[patches[i].first][patches[i].second]));
    return matched;
}

Line ransac(std::vector<std::pair<int, int> > points, int patch_w, double threshold)
{
    int best_subset = 0;
    int best_p1 = 0, best_p2 = 1;
    double mx, my, cx, cy;
    srand(time(NULL));
    for(int i = 0; i < RANSAC_ITER; i++)
    {
        int p1, p2;
        p1 = rand()%points.size();
        do
            p2 = rand()%points.size();
        while(p1 == p2);

        double dx = points[p2].first - points[p1].first;
        double dy = points[p2].second - points[p1].second;
        double xy = points[p2].first * points[p1].second - points[p1].first * points[p2].second;
        double dl = sqrt(dx*dx + dy*dy);

        int subset = 0;
        for(unsigned int j = 0; j < points.size(); j++)
        {
            if(fabs(dy*(points[j].first + patch_w/2.0) - dx*(points[j].second + patch_w/2.0) + xy)/dl < threshold)
                subset++;
        }

        if(subset > best_subset)
        {
            best_subset = subset;
            best_p1 = p1;
            best_p2 = p2;
            mx = 0;
            my = 0;
            cx = 0;
            cy = 0;
            for(unsigned int j = 0; j < points.size(); j++)
                if(fabs(dy*(points[j].first + patch_w/2.0) - dx*(points[j].second + patch_w/2.0) + xy)/dl < threshold)
                {
                    mx += points[j].first;
                    my += points[j].second;
                }
            mx = mx/subset;
            my = my/subset;
            for(unsigned int j = 0; j < points.size(); j++)
                if(fabs(dy*(points[j].first + patch_w/2.0) - dx*(points[j].second + patch_w/2.0) + xy)/dl < threshold)
                {
                    if(points[j].second - my < 0)
                        cx -= points[j].first - mx;
                    else
                        cx += points[j].first - mx;
                    if(points[j].first - mx < 0)
                        cy -= points[j].second - my;
                    else
                        cy += points[j].second - my;
                }
        }
    }
    int xmin = points[0].first;
    int xmax = points[0].first;
    for(unsigned int j = 0; j < points.size(); j++)
    {
        if(points[j].first < xmin)
            xmin = points[j].first;
        else if(points[j].first > xmax)
            xmax = points[j].first;
    }
    Line line(xmin, (xmin - mx)/cx*cy + my);
    line.extend(xmax, (xmax - mx)/cx*cy + my);

    return line;
}
