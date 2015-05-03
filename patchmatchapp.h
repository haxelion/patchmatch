#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

class PatchMatchApp
{
public:
    GtkWidget *menu_file_open;
    GtkWidget *menu_file_save;
    GtkWidget *menu_file_save_as;
    GtkWidget *menu_file_quit;
    GtkWidget *drawing_area;
    GtkWidget *main_window;
    GdkPixbuf *image;
    char *filename;

    static void cb_menu_file_open(GtkWidget* widget, gpointer *app);
    static void cb_menu_file_save(GtkWidget* widget, gpointer *app);
    static void cb_menu_file_save_as(GtkWidget* widget, gpointer *app);
    static void cb_menu_file_quit(GtkWidget* widget, gpointer *app);
    static gboolean cb_draw(GtkWidget *widget, cairo_t *cr, gpointer *app);

    PatchMatchApp();
    ~PatchMatchApp();
    void openFile(const char *filename);
    void saveFile();
    void saveFileAs(const char *filename);
};