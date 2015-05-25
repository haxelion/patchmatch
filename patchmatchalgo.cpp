#include "patchmatchalgo.h"

PatchMatchAlgo::PatchMatchAlgo(QImage *source, QImage *target, std::vector<Zone*> *zones, double xscale, double yscale, int patch_w, int patchmatch_iterations, int em_iterations)
{
    this->em_iterations = em_iterations;
    this->patchmatch_iterations = patchmatch_iterations;
    this->patch_w = patch_w;
    canceled = false;
    this->zones = new std::vector<Zone*>(*zones);
    this->source = new QImage(*source);
    this->target = new QImage(target->scaled(target->width()*xscale, target->height()*yscale, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    QPainter painter;
    painter.begin(this->target);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    for(unsigned int i = 0; i < zones->size(); i++)
    {
        if(zones->at(i)->type == FIXEDZONE)
        {
            zones->at(i)->draw(&painter, source, false);
        }
    }
    painter.end();

    int target_height = this->target->height();
    int target_width = this->target->width();
    annx = new int*[target_width];
    anny = new int*[target_width];
    annd = new int*[target_width];
    for(int i = 0; i < target_width; i++)
    {
        annx[i] = new int[target_height];
        anny[i] = new int[target_height];
        annd[i] = new int[target_height];
    }
}

PatchMatchAlgo::~PatchMatchAlgo()
{
    for(int i = 0; i < target->width(); i++)
    {
        delete annx[i];
        delete anny[i];
        delete annd[i];
    }
    delete annx;
    delete anny;
    delete annd;
    delete source;
    delete target;
    delete zones;
}

void PatchMatchAlgo::run()
{

    QImage source_scaled, target_scaled;
    int target_width = target->width();
    int target_height = target->height();
    int source_width = source->width();
    int source_height = source->height();
    // Compute starting scale
    scale = 1 << (int)(log2(min(min(source_width, source_height),min(target_width, target_height))/patch_w)-1);
    // Compute total work to be done (pixel count)
    total_work = 0;
    work_done = 0;
    for(int i = scale; i >= 1; i = i/2)
    {
        total_work += em_iterations * target_width/i * target_height/i;
    }
    bool first_loop = true;
    while(scale >= 1 && !canceled)
    {
        target_scaled = target->scaled(target_width/scale, target_height/scale, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        source_scaled = source->scaled(source->width()/scale, source->height()/scale, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        if(first_loop)
        {
            first_loop = false;
            randomANN(&source_scaled, &target_scaled);
        }
        else
        {
            rescaleANN(&source_scaled, &target_scaled);
            patchVoting(&source_scaled, &target_scaled);
            enforceFixedZone(source, &target_scaled);
        }
        for(int i = 0; i < em_iterations && !canceled; i++)
        {
            patchMatch(&source_scaled, &target_scaled);
            patchVoting(&source_scaled, &target_scaled);
            if(scale != 1)
                enforceFixedZone(source, &target_scaled);
            work_done += target_width/scale * target_height/scale;
            // Atomic replacement of reconstructed pixbuf
            emit progressed(target_scaled);
            emit progressed(work_done / total_work);
        }
        scale = scale >> 1;
    }
    emit finished();
}

void PatchMatchAlgo::stop()
{
    canceled = true;
}

inline int PatchMatchAlgo::distance(QImage *a, QImage *b, int ax, int ay, int bx, int by)
{
    int d = 0;
    int a_rowstride = a->bytesPerLine();
    int b_rowstride = b->bytesPerLine();
    unsigned char *a_p = a->bits();
    unsigned char *b_p = b->bits();
    a_p += ay*a_rowstride + ax*4;
    b_p += by*b_rowstride + bx*4;
    for(int i = 0; i < patch_w; i++)
    {
        for(int j = 0; j < patch_w; j++)
        {
            d += (a_p[0]-b_p[0])*(a_p[0]-b_p[0]) +
                 (a_p[1]-b_p[1])*(a_p[1]-b_p[1]) +
                 (a_p[2]-b_p[2])*(a_p[2]-b_p[2]);
            a_p += 4;
            b_p += 4;
        }
        a_p += a_rowstride - patch_w*4;
        b_p += b_rowstride - patch_w*4;
    }
    return d;
}

inline void PatchMatchAlgo::randomANN(QImage *source, QImage *target)
{
    int target_height = target->height() - patch_w;
    int target_width = target->width() - patch_w;
    int source_height = source->height() - patch_w;
    int source_width = source->width() - patch_w;

    srand(time(NULL));

    for(int i = 0; i < target_width; i++)
        for(int j = 0; j < target_height; j++)
        {
            do
            {
                annx[i][j] = rand()%source_width;
                anny[i][j] = rand()%source_height;
            }
            while(!isReplaceSatisfied(i, j, annx[i][j], anny[i][j]));
            annd[i][j] = distance(source, target, annx[i][j], anny[i][j], i, j);
        }
}

inline void PatchMatchAlgo::rescaleANN(QImage *source, QImage *target)
{
    int target_height = target->height() - patch_w;
    int target_width = target->width() - patch_w;
    int source_height = source->height() - patch_w;
    int source_width = source->width() - patch_w;
    int inner_width = (target->width()/2 - patch_w)*2;
    int inner_height = (target->height()/2 - patch_w)*2;

    for(int i = inner_width-1; i >= 0; i--)
        for(int j = inner_height-1; j >= 0; j--)
        {
                    annx[i][j] = 2*annx[i/2][j/2] + i%2;
                    anny[i][j] = 2*anny[i/2][j/2] + j%2;
                    annd[i][j] = INT_MAX;
        }

    for(int i = target_width - inner_width; i >= 1; i--)
        for(int j = 0; j < inner_height; j++)
        {
            annx[target_width-i][j] = min(source_width, annx[target_width-i-patch_w][j]+patch_w);
            anny[target_width-i][j] = min(source_height, anny[target_width-i-patch_w][j]);
            annd[target_width-i][j] = INT_MAX;
        }
    for(int i = 0; i < inner_width; i++)
        for(int j = target_height - inner_height; j >= 1; j--)
        {
            annx[i][target_height-j] = min(source_width, annx[i][target_height-j-patch_w]);
            anny[i][target_height-j] = min(source_height, anny[i][target_height-j-patch_w] + patch_w);
            annd[i][target_height-j] = INT_MAX;
        }
    for(int i = target_width - inner_width; i >= 1; i--)
        for(int j = target_height - inner_height; j >= 1; j--)
        {
            annx[target_width-i][target_height-j] = min(source_width, annx[target_width-i-patch_w][target_height-j-patch_w] + patch_w);
            anny[target_width-i][target_height-j] = min(source_height, anny[target_width-i-patch_w][target_height-j-patch_w] + patch_w);
            annd[target_width-i][target_height-j] = INT_MAX;
        }
}

inline void PatchMatchAlgo::patchVoting(QImage *source, QImage *target)
{
    int s_rowstride = source->bytesPerLine();
    int t_rowstride = target->bytesPerLine();
    unsigned char *s_pixels = source->bits();
    unsigned char *t_pixels = target->bits();
    int target_height = target->height();
    int target_width = target->width();

    int *votes = new int[target_width * target_height * 3];
    int *nvotes = new int[target_width * target_height];

    // Initialize with zeroes
    for(int i = 0; i < target_width * target_height*3; i++)
        votes[i] = 0;
    for(int i = 0; i < target_width * target_height; i++)
        nvotes[i] = 0;
    
    // Perform patch voting
    for(int i = 0; i < target_width - patch_w; i++)
        for(int j = 0; j < target_height - patch_w; j++)
        {
            for(int k = 0; k < patch_w; k++)
                for(int l = 0; l < patch_w; l++)
                {
                    unsigned char *s_p = s_pixels + (anny[i][j]+l)*s_rowstride + (annx[i][j]+k)*4;
                    int* v = votes + ((j+l)*target_width + i + k)*3;
                    v[0] += s_p[0];
                    v[1] += s_p[1];
                    v[2] += s_p[2];
                    nvotes[(j+l)*target_width + i + k]++;
                }
        }

    // Average voting results
    for(int i = 0; i < target_width; i++)
        for(int j = 0; j < target_height; j++)
        {
            unsigned char *t_p = t_pixels + j*t_rowstride + i*4;
            int* v = votes + (j*target_width + i)*3;
            int nv = nvotes[j*target_width + i]; 
            if(nv != 0)
            {
                t_p[0] = v[0]/nv;
                t_p[1] = v[1]/nv;
                t_p[2] = v[2]/nv;
            }
        }
}

inline void PatchMatchAlgo::patchMatch(QImage *source, QImage *target)
{
    int target_height = target->height();
    int target_width = target->width();
    int source_height = source->height();
    int source_width = source->width();

    for(int i = 0; i < patchmatch_iterations; i++)
    {
        int xstart, xend, xchange;
        int ystart, yend, ychange;
        if(i%2 == 0)
        {
            xstart = 0;
            xend = target_width - patch_w;
            xchange = 1;
            ystart = 0;
            yend = target_height - patch_w;
            ychange = 1;
        }
        else
        {
            xend = -1;
            xstart = target_width - patch_w - 1;
            xchange = -1;
            yend = -1;
            ystart = target_height - patch_w - 1;
            ychange = -1;
        }

        for(int x = xstart; x != xend; x += xchange)
        {
            for(int y = ystart; y != yend; y += ychange)
            {
                int xbest = annx[x][y];
                int ybest = anny[x][y];
                int dbest = annd[x][y];

                if(x - xchange < target_width - patch_w && x - xchange >= 0)
                {
                    int xp = annx[x-xchange][y] + xchange;
                    int yp = anny[x-xchange][y];
                    if(xp < source_width - patch_w && xp >= 0 && 
                       isReplaceSatisfied(x, y, xp, yp))
                    {
                        int d = distance(source, target, xp, yp, x, y);
                        if(d < dbest)
                        {
                            xbest = xp;
                            ybest = yp;
                            dbest = d;
                        }
                    }
                }

                if(y - ychange < target_height - patch_w && y - ychange >= 0)
                {
                    int xp = annx[x][y - ychange];
                    int yp = anny[x][y - ychange] + ychange;
                    if(yp < source_height - patch_w && yp >= 0 && 
                       isReplaceSatisfied(x, y, xp, yp))
                    {
                        int d = distance(source, target, xp, yp, x, y);
                        if(d < dbest)
                        {
                            xbest = xp;
                            ybest = yp;
                            dbest = d;
                        }
                    }
                }


                int mag = max(source_height, source_width);
                while(mag >= 1)
                {
                    int xmin = max(xbest - mag, 0);
                    int xmax = min(xbest + mag, source_width - patch_w - 1);
                    int ymin = max(ybest - mag, 0);
                    int ymax = min(ybest + mag, source_height - patch_w - 1);
                    int xp, yp;
                    do
                    {
                        xp = xmin + rand()%(xmax - xmin);
                        yp = ymin + rand()%(ymax - ymin);
                    }
                    while(!isReplaceSatisfied(x, y, xp, yp));
                    int d = distance(source, target, xp, yp, x, y);
                    if(d < dbest)
                    {
                        xbest = xp;
                        ybest = yp;
                        dbest = d;
                    }
                    mag = mag/2;
                }

                annx[x][y] = xbest;
                anny[x][y] = ybest;
                annd[x][y] = dbest;
            }
        }
    }
}

inline void PatchMatchAlgo::enforceFixedZone(QImage *source, QImage *target)
{
    // Enforce fixed zone constraints
    QPainter painter;
    painter.begin(target);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.scale(1.0/scale, 1.0/scale);
    for(unsigned int i = 0; i < zones->size(); i++)
    {
        if(zones->at(i)->type == FIXEDZONE)
        {
            zones->at(i)->draw(&painter, source, false);
        }
    }
    painter.end();
}

inline bool PatchMatchAlgo::isReplaceSatisfied(int sx, int sy, int tx, int ty)
{
    for(unsigned int i = 0; i < zones->size(); i++)
    {
        if(zones->at(i)->type == REPLACEZONE && 
           zones->at(i)->contains(tx + patch_w/2, ty + patch_w/2, scale) &&
           zones->at(i)->contains(sx + patch_w/2, sy + patch_w/2, scale))
            return false;
            
    }
    return true;
}
