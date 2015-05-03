#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

enum TOOL
{
    NONE,
    RESHUFFLE_RECTANGLE,
    RESHUFFLE_FREE_HAND,
};

struct FixedZone
{
    int src_x, src_y, src_heig;
};

class PatchMatchApp
{
public:
    GtkWidget *menu_file_open;
    GtkWidget *menu_file_save;
    GtkWidget *menu_file_save_as;
    GtkWidget *menu_file_quit;
    GtkWidget *drawing_area;
    GtkWidget *main_window;
    GdkPixbuf *source;
    GdkPixbuf *target;
    char *filename;
    std::vector<FixedZone> *fixed_zones;

    static void cb_menu_file_open(GtkWidget* widget, gpointer app);
    static void cb_menu_file_save(GtkWidget* widget, gpointer app);
    static void cb_menu_file_save_as(GtkWidget* widget, gpointer app);
    static void cb_menu_file_quit(GtkWidget* widget, gpointer app);
    static void cb_button_pressed(GtkWidget *widget, GdkEvent *event, gpointer app);
    static void cb_button_released(GtkWidget *widget, GdkEvent *event, gpointer app);
    static void cb_motion_notify(GtkWidget *widget, GdkEvent *event, gpointer app);
    static gboolean cb_draw(GtkWidget *widget, cairo_t *cr, gpointer app);

    PatchMatchApp();
    ~PatchMatchApp();
    void openFile(const char *filename);
    void saveFile();
    void saveFileAs(const char *filename);
};
