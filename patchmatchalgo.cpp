#include "patchmatchalgo.h"

PatchMatchAlgo::PatchMatchAlgo()
{
    em_iteration = 8;
    patchmatch_iteration = 5;
    patch_w = 7;
    threshold = 64;
    thread = NULL;
    source = NULL;
    target = NULL;
    reconstructed = NULL;
    zones = NULL;
}

void PatchMatchAlgo::run(cairo_surface_t *source, cairo_surface_t *target, std::vector<Zone*> *zones, std::vector<Line*> *lines, double xscale, double yscale)
{
    if(thread != NULL)
        return;
    if(this->source != NULL)
        cairo_surface_destroy(this->source);
    if(this->target != NULL)
        cairo_surface_destroy(this->target);
    if(this->zones != NULL)
        delete this->zones;

    this->source = cairo_image_surface_create(
        CAIRO_FORMAT_RGB24, 
        cairo_image_surface_get_width(source), 
        cairo_image_surface_get_height(source));
    cairo_t *cr = cairo_create(this->source);
    cairo_set_source_surface(cr, source, 0, 0);
    cairo_paint(cr);
    cairo_surface_flush(this->source);
    cairo_destroy(cr);

    this->target = cairo_image_surface_create(
        CAIRO_FORMAT_RGB24, 
        cairo_image_surface_get_width(target)*xscale, 
        cairo_image_surface_get_height(target)*yscale);
    cr = cairo_create(this->target);
    cairo_save(cr);
    cairo_scale(cr, xscale, yscale);
    cairo_set_source_surface(cr, target, 0, 0);
    cairo_paint(cr);
    cairo_restore(cr);
    for(unsigned int i = 0; i < zones->size(); i++)
    {
        zones->at(i)->draw(cr, source, 1, false);
    }
    cairo_surface_flush(this->target);
    cairo_destroy(cr);

    this->zones = new std::vector<Zone*>(*zones);
    this->lines = new std::vector<Line*>(*lines);
    this->terminate = false;
    thread = g_thread_new("patchmatch", PatchMatchAlgo::threadFunction, this);
}

void PatchMatchAlgo::stop()
{
    this->terminate = true;
}

gpointer PatchMatchAlgo::threadFunction(gpointer data)
{
    PatchMatchAlgo *self = (PatchMatchAlgo*) data;
    cairo_surface_t *source_scaled = NULL;
    cairo_surface_t *target_scaled = NULL;
    int target_height = cairo_image_surface_get_height(self->target);
    int target_width = cairo_image_surface_get_width(self->target);
    int **annx, **anny, **annd;
    annx = new int*[target_width];
    anny = new int*[target_width];
    annd = new int*[target_width];
    for(int i = 0; i < target_width; i++)
    {
        annx[i] = new int[target_height];
        anny[i] = new int[target_height];
        annd[i] = new int[target_height];
    }
    // Compute starting scale
    int scale = 1 << (int)(log2(min(target_width, target_height)/self->patch_w)-1);
    // Compute total work to be done (pixel count)
    self->total_work = 0;
    self->work_done = 0;
    self->done = false;
    for(int i = scale; i >= 1; i = i/2)
    {
        self->total_work += self->em_iteration * target_width/i * target_height/i;
    }
    bool first_loop = true;
    while(scale >= 1 && !self->terminate)
    {
        if(source_scaled != NULL)
            cairo_surface_destroy(source_scaled);
        if(target_scaled != NULL)
            cairo_surface_destroy(target_scaled);
        target_scaled = scaleSurface(self->target, scale); 
        source_scaled = scaleSurface(self->source, scale); 
        if(first_loop)
        {
            first_loop = false;
            randomANN(source_scaled, target_scaled, self->zones, annx, anny, annd, self->patch_w, scale);
        }
        else
        {
            rescaleANN(source_scaled, target_scaled, annx, anny, annd, self->patch_w);
            patchVoting(source_scaled, target_scaled, self->zones, annx, anny, self->patch_w);
            enforceFixedZone(self->source, target_scaled, self->zones, scale);
        }
        for(int i = 0; i < self->em_iteration && !self->terminate; i++)
        {
            patchMatch(source_scaled, target_scaled, self->zones, annx, anny, annd, self->patch_w, self->patchmatch_iteration, scale);
            enforceLineConstraints(annx, anny, self->lines, target_width, target_height, self->patch_w, scale, self->threshold);
            patchVoting(source_scaled, target_scaled, self->zones, annx, anny, self->patch_w);
            if(scale != 1)
                enforceFixedZone(self->source, target_scaled, self->zones, scale);
            self->work_done += target_width/scale * target_height/scale;
            // Atomic replacement of reconstructed pixbuf
            cairo_surface_t *new_surface = scaleSurface(target_scaled, 1);
            cairo_surface_t *old_surface = self->reconstructed;
            self->reconstructed = new_surface;
            cairo_surface_destroy(old_surface);
        }
        scale = scale >> 1;
    }
    self->done = true;
    cairo_surface_destroy(source_scaled);
    cairo_surface_destroy(target_scaled);
    for(int i = 0; i < target_width; i++)
    {
        delete annx[i];
        delete anny[i];
        delete annd[i];
    }
    delete annx;
    delete anny;
    delete annd;
    g_thread_unref(self->thread);
    self->thread = NULL;
    return NULL;
}

inline cairo_surface_t * scaleSurface(cairo_surface_t *surface, int scale)
{
    cairo_surface_t *scaled = cairo_image_surface_create(
        CAIRO_FORMAT_RGB24, 
        (cairo_image_surface_get_width(surface) + scale - 1)/scale, 
        (cairo_image_surface_get_height(surface) + scale -1)/scale);
    cairo_t *cr = cairo_create(scaled);
    cairo_scale(cr, 1.0/scale, 1.0/scale);
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);
    cairo_surface_flush(scaled);
    cairo_destroy(cr);
    return scaled;
}

inline int distance(cairo_surface_t *source, cairo_surface_t *target, int sx, int sy, int tx, int ty, int patch_w)
{
    int d = 0;
    int s_rowstride = cairo_image_surface_get_stride(source);
    int t_rowstride = cairo_image_surface_get_stride(target);
    unsigned char *s_p = cairo_image_surface_get_data(source);
    unsigned char *t_p = cairo_image_surface_get_data(target);
    s_p += sy*s_rowstride + sx*4;
    t_p += ty*t_rowstride + tx*4;
    for(int i = 0; i < patch_w; i++)
    {
        for(int j = 0; j < patch_w; j++)
        {
            d += (s_p[0]-t_p[0])*(s_p[0]-t_p[0]) +
                 (s_p[1]-t_p[1])*(s_p[1]-t_p[1]) +
                 (s_p[2]-t_p[2])*(s_p[2]-t_p[2]);
            s_p += 4;
            t_p += 4;
        }
        s_p += s_rowstride - patch_w*4;
        t_p += t_rowstride - patch_w*4;
    }
    return d;
}

inline void randomANN(cairo_surface_t *source, cairo_surface_t *target, std::vector<Zone*> *zones, int **annx, int **anny, int **annd, int patch_w, int scale)
{
    int target_height = cairo_image_surface_get_height(target) - patch_w;
    int target_width = cairo_image_surface_get_width(target) - patch_w;
    int source_height = cairo_image_surface_get_height(source) - patch_w;
    int source_width = cairo_image_surface_get_width(source) - patch_w;

    srand(time(NULL));

    for(int i = 0; i < target_width; i++)
        for(int j = 0; j < target_height; j++)
        {
            do
            {
                annx[i][j] = rand()%source_width;
                anny[i][j] = rand()%source_height;
            }
            while(!isReplaceSatisfied(zones, i, j, annx[i][j], anny[i][j], patch_w, scale));
            annd[i][j] = distance(source, target, annx[i][j], anny[i][j], i, j, patch_w);
        }
}

inline void rescaleANN(cairo_surface_t *source, cairo_surface_t *target, int **annx, int **anny, int **annd, int patch_w)
{
    int target_height = cairo_image_surface_get_height(target) - patch_w;
    int target_width = cairo_image_surface_get_width(target) - patch_w;
    int source_height = cairo_image_surface_get_height(source) - patch_w;
    int source_width = cairo_image_surface_get_width(source) - patch_w;

    for(int i = target_width - patch_w - 1; i >= 0; i--)
        for(int j = target_height - patch_w - 1; j >= 0; j--)
        {
                    annx[i][j] = 2*annx[i/2][j/2] + i%2;
                    anny[i][j] = 2*anny[i/2][j/2] + j%2;
                    annd[i][j] = INT_MAX;
        }

    for(int i = 1; i <= patch_w; i++)
        for(int j = 0; j < target_height - patch_w; j++)
        {
            annx[target_width-i][j] = min(source_width, annx[target_width-i-patch_w][j]+patch_w);
            anny[target_width-i][j] = min(source_height, anny[target_width-i-patch_w][j]);
            annd[target_width-i][j] = INT_MAX;
        }
    for(int i = 0; i < target_width - patch_w; i++)
        for(int j = 1; j <= patch_w; j++)
        {
            annx[i][target_height-j] = min(source_width, annx[i][target_height-j-patch_w]);
            anny[i][target_height-j] = min(source_height, anny[i][target_height-j-patch_w] + patch_w);
            annd[i][target_height-j] = INT_MAX;
        }
    for(int i = 1; i <= patch_w; i++)
        for(int j = 1; j <= patch_w; j++)
        {
            annx[target_width-i][target_height-j] = min(source_width, annx[target_width-i-patch_w][target_height-j-patch_w] + patch_w);
            anny[target_width-i][target_height-j] = min(source_height, anny[target_width-i-patch_w][target_height-j-patch_w] + patch_w);
            annd[target_width-i][target_height-j] = INT_MAX;
        }
}

inline void patchVoting(cairo_surface_t *source, cairo_surface_t *target, std::vector<Zone*> *zones, int **annx, int **anny, int patch_w)
{
    int s_rowstride = cairo_image_surface_get_stride(source);
    int t_rowstride = cairo_image_surface_get_stride(target);
    int target_height = cairo_image_surface_get_height(target);
    int target_width = cairo_image_surface_get_width(target);

    unsigned char *s_pixels = cairo_image_surface_get_data(source);
    unsigned char *t_pixels = cairo_image_surface_get_data(target);
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

inline void patchMatch(cairo_surface_t *source, cairo_surface_t *target, std::vector<Zone*> *zones, int **annx, int **anny, int **annd, int patch_w, int iter, int scale)
{
    int target_height = cairo_image_surface_get_height(target);
    int target_width = cairo_image_surface_get_width(target);
    int source_height = cairo_image_surface_get_height(source);
    int source_width = cairo_image_surface_get_width(source);

    for(int i = 0; i < iter; i++)
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
                       isReplaceSatisfied(zones, x, y, xp, yp, patch_w, scale))
                    {
                        int d = distance(source, target, xp, yp, x, y, patch_w);
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
                       isReplaceSatisfied(zones, x, y, xp, yp, patch_w, scale))
                    {
                        int d = distance(source, target, xp, yp, x, y, patch_w);
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
                    while(!isReplaceSatisfied(zones, x, y, xp, yp, patch_w, scale));
                    int d = distance(source, target, xp, yp, x, y, patch_w);
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

inline void enforceFixedZone(cairo_surface_t *source, cairo_surface_t *target, std::vector<Zone*> *zones, int scale)
{
    // Enforce fixed zone constraints
    cairo_t *cr = cairo_create(target);
    for(unsigned int i = 0; i < zones->size(); i++)
    {
        if(zones->at(i)->type == FIXEDZONE)
        {
            zones->at(i)->draw(cr, source, scale, false);
        }
    }
    cairo_surface_flush(target);
    cairo_destroy(cr);
}

inline void enforceLineConstraints(int **annx, int **anny, std::vector<Line*> *lines, int width, int height, int patch_w, int scale, double threshold)
{
    for(unsigned int i = 0; i < lines->size(); i++)
    {
        lines->at(i)->applyLineConstraint(annx, anny, width/scale, height/scale, patch_w, scale, threshold);
    }
}

inline bool isReplaceSatisfied(std::vector<Zone*> *zones, int sx, int sy, int tx, int ty, int patch_w, int scale)
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
