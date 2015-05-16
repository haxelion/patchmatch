#include "patchmatchapp.h"
#include "patchmatchalgo.h"

PatchMatchAlgo::PatchMatchAlgo()
{
    em_iteration = 8;
    patchmatch_iteration = 5;
    patch_w = 7;
    thread = NULL;
    source = NULL;
    target = NULL;
    reconstructed = NULL;
    zones = NULL;
}

void PatchMatchAlgo::run(GdkPixbuf *source, GdkPixbuf *target, std::vector<Zone> *zones)
{
    if(thread != NULL)
        return;
    if(this->source != NULL)
        g_object_unref(this->source);
    if(this->target != NULL)
        g_object_unref(this->target);
    if(this->zones != NULL)
        delete this->zones;
    this->source = gdk_pixbuf_copy(source);
    cairo_surface_t *surface = cairo_image_surface_create(
        CAIRO_FORMAT_RGB24, 
        gdk_pixbuf_get_width(target), 
        gdk_pixbuf_get_height(target));
    cairo_t *cr = cairo_create(surface);
    gdk_cairo_set_source_pixbuf(cr, target, 0, 0);
    cairo_paint(cr);
    for(unsigned int i = 0; i < zones->size(); i++)
    {
        cairo_rectangle(cr, zones->at(i).dst_x, zones->at(i).dst_y,
            zones->at(i).src_width, zones->at(i).src_height);
        cairo_clip(cr);
        gdk_cairo_set_source_pixbuf(cr, target,
            zones->at(i).dst_x - zones->at(i).src_x, 
            zones->at(i).dst_y - zones->at(i).src_y);
        cairo_paint(cr);
        cairo_reset_clip(cr);
    }
    cairo_surface_flush(surface);
    this->target = gdk_pixbuf_get_from_surface(surface, 0, 0, 
        cairo_image_surface_get_width(surface), 
        cairo_image_surface_get_height(surface));
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    this->zones = new std::vector<Zone>(*zones);
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
    GdkPixbuf *source_scaled = NULL;
    GdkPixbuf *target_scaled = NULL;
    int target_height = gdk_pixbuf_get_height(self->target);
    int target_width = gdk_pixbuf_get_width(self->target);
    int source_height = gdk_pixbuf_get_height(self->source);
    int source_width = gdk_pixbuf_get_width(self->source);
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
            g_object_unref(source_scaled);
        if(target_scaled != NULL)
            g_object_unref(target_scaled);
        target_scaled = gdk_pixbuf_scale_simple(self->target, target_width/scale, 
            target_height/scale, GDK_INTERP_BILINEAR);
        source_scaled = gdk_pixbuf_scale_simple(self->source, source_width/scale, 
            source_height/scale, GDK_INTERP_BILINEAR);
        if(first_loop)
        {
            first_loop = false;
            randomANN(source_scaled, target_scaled, annx, anny, annd, self->patch_w);
        }
        else
        {
            rescaleANN(source_scaled, target_scaled, annx, anny, annd, self->patch_w);
            patchVoting(source_scaled, target_scaled, self->zones, annx, anny, self->patch_w, scale);
        }
        for(int i = 0; i < self->em_iteration && !self->terminate; i++)
        {
            patchMatch(source_scaled, target_scaled, self->zones, annx, anny, annd, self->patch_w, scale, self->patchmatch_iteration);
            patchVoting(source_scaled, target_scaled, self->zones, annx, anny, self->patch_w, scale);
            self->work_done += target_width/scale * target_height/scale;
            // Atomic replacement of reconstructed pixbuf
            GdkPixbuf *new_pixbuf = gdk_pixbuf_copy(target_scaled);
            GdkPixbuf *old_pixbuf = self->reconstructed;
            self->reconstructed = new_pixbuf;
            g_object_unref(old_pixbuf);
        }
        scale = scale >> 1;
    }
    self->done = true;
    g_object_unref(source_scaled);
    g_object_unref(target_scaled);
    g_thread_unref(self->thread);
    self->thread = NULL;
    return NULL;
}

inline int distance(GdkPixbuf *source, GdkPixbuf *target, int sx, int sy, int tx, int ty, int patch_w)
{
    int d = 0;
    int s_rowstride = gdk_pixbuf_get_rowstride(source);
    int t_rowstride = gdk_pixbuf_get_rowstride(target);
    guchar *s_p = gdk_pixbuf_get_pixels(source);
    guchar *t_p = gdk_pixbuf_get_pixels(target);
    s_p += sy*s_rowstride + sx*3;
    t_p += ty*t_rowstride + tx*3;
    for(int i = 0; i < patch_w; i++)
    {
        for(int j = 0; j < patch_w; j++)
        {
            d += (s_p[0]-t_p[0])*(s_p[0]-t_p[0]) +
                 (s_p[1]-t_p[1])*(s_p[1]-t_p[1]) +
                 (s_p[2]-t_p[2])*(s_p[2]-t_p[2]);
            s_p += 3;
            t_p += 3;
        }
        s_p += s_rowstride - patch_w*3;
        t_p += t_rowstride - patch_w*3;
    }
    return d;
}

inline void randomANN(GdkPixbuf *source, GdkPixbuf *target, int **annx, int **anny, int **annd, int patch_w)
{
    int target_height = gdk_pixbuf_get_height(target) - patch_w;
    int target_width = gdk_pixbuf_get_width(target) - patch_w;
    int source_height = gdk_pixbuf_get_height(source) - patch_w;
    int source_width = gdk_pixbuf_get_width(source) - patch_w;

    srand(time(NULL));

    for(int i = 0; i < target_width; i++)
        for(int j = 0; j < target_height; j++)
        {
            annx[i][j] = rand()%source_width;
            anny[i][j] = rand()%source_height;
            annd[i][j] = distance(source, target, annx[i][j], anny[i][j], i, j, patch_w);
        }
}

inline void rescaleANN(GdkPixbuf *source, GdkPixbuf *target, int **annx, int **anny, int **annd, int patch_w)
{
    int target_height = gdk_pixbuf_get_height(target) - patch_w;
    int target_width = gdk_pixbuf_get_width(target) - patch_w;
    int source_height = gdk_pixbuf_get_height(source) - patch_w;
    int source_width = gdk_pixbuf_get_width(source) - patch_w;

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

inline void patchVoting(GdkPixbuf *source, GdkPixbuf *target, std::vector<Zone> *zones, int **annx, int **anny, int patch_w, int scale)
{
    int s_rowstride = gdk_pixbuf_get_rowstride(source);
    int t_rowstride = gdk_pixbuf_get_rowstride(target);
    int target_height = gdk_pixbuf_get_height(target);
    int target_width = gdk_pixbuf_get_width(target);
    int source_height = gdk_pixbuf_get_height(source);
    int source_width = gdk_pixbuf_get_width(source);

    guchar *s_pixels = gdk_pixbuf_get_pixels(source);
    guchar *t_pixels = gdk_pixbuf_get_pixels(target);
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
                    guchar *s_p = s_pixels + (anny[i][j]+l)*s_rowstride + (annx[i][j]+k)*3;
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
            guchar *t_p = t_pixels + j*t_rowstride + i*3;
            int* v = votes + (j*target_width + i)*3;
            int nv = nvotes[j*target_width + i]; 
            if(nv != 0)
            {
                t_p[0] = v[0]/nv;
                t_p[1] = v[1]/nv;
                t_p[2] = v[2]/nv;
            }
        }

    // Enforce fixed zone constraints
    for(unsigned int i = 0; i < zones->size(); i++)
    {
        if(zones->at(i).type == FIXEDZONE)
        {
            Zone z = zones->at(i).scale(scale);
            int xs = max(max(-z.src_x, -z.dst_x), 0);
            int xe = min(min(target_width - z.dst_x, source_width - z.src_x), z.src_width);
            int ys = max(max(-z.src_y, -z.dst_y), 0);
            int ye = min(min(target_height - z.dst_y, source_height - z.src_y), z.src_height);
            for(int x = xs; x < xe; x++)
                for(int y = ys; y < ye; y++)
                {
                    guchar *s_p = s_pixels + (z.src_y + y)*s_rowstride + (z.src_x + x)*3;
                    guchar *t_p = t_pixels + (z.dst_y + y)*t_rowstride + (z.dst_x + x)*3;
                    t_p[0] = s_p[0];
                    t_p[1] = s_p[1];
                    t_p[2] = s_p[2];
                }

        }
    }
}

inline void patchMatch(GdkPixbuf *source, GdkPixbuf *target, std::vector<Zone> *zones, int **annx, int **anny, int **annd, int patch_w, int scale, int iter)
{
    int target_height = gdk_pixbuf_get_height(target);
    int target_width = gdk_pixbuf_get_width(target);
    int source_height = gdk_pixbuf_get_height(source);
    int source_width = gdk_pixbuf_get_width(source);

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
                    if(xp < source_width - patch_w && xp >= 0)
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
                    if(yp < source_height - patch_w && yp >= 0)
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
                    int xp = xmin + rand()%(xmax - xmin);
                    int yp = ymin + rand()%(ymax - ymin);
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
