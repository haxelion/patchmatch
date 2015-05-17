#ifndef PATCHMATCHAPP_H 
#define PATCHMATCHAPP_H

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "zone.h"
#include "patchmatchalgo.h"

enum TOOL
{
    TOOL_NONE,
    TOOL_MOVE,
    TOOL_DELETE,
    TOOL_RESHUFFLE_RECTANGLE,
    TOOL_RESHUFFLE_FREE_HAND,
};

struct Move
{
    int zone;
    int dx, dy;
};

class PatchMatchApp
{
public:
    GtkWidget *menu_file_open;
    GtkWidget *menu_file_save;
    GtkWidget *menu_file_save_as;
    GtkWidget *menu_file_quit;
    GtkWidget *tool_move;
    GtkWidget *tool_delete;
    GtkWidget *tool_reshuffle_rectangle;
    GtkWidget *tool_reshuffle_free;
    GtkWidget *tool_process;
    GtkWidget *drawing_area;
    GtkWidget *main_window;
    GtkWidget *progress_window;
    GtkWidget *level_bar;
    cairo_surface_t *source;
    cairo_surface_t *target;
    double scale;
    char *filename;
    std::vector<Zone*> *zones;
    bool button_pressed;
    TOOL active_tool;
    Move move;
    PatchMatchAlgo *algo;

    static void cb_menu_file_open(GtkWidget* widget, gpointer app);
    static void cb_menu_file_save(GtkWidget* widget, gpointer app);
    static void cb_menu_file_save_as(GtkWidget* widget, gpointer app);
    static void cb_menu_file_quit(GtkWidget* widget, gpointer app);
    static void cb_toolbar_clicked(GtkWidget* widget, gpointer app);
    static void cb_button_pressed(GtkWidget *widget, GdkEvent *event, gpointer app);
    static void cb_button_released(GtkWidget *widget, GdkEvent *event, gpointer app);
    static void cb_motion_notify(GtkWidget *widget, GdkEvent *event, gpointer app);
    static void cb_patchmatch_canceled(GtkWidget *widget, GdkEvent *event, gpointer app);
    static gboolean cb_draw(GtkWidget *widget, cairo_t *cr, gpointer app);
    static gboolean cb_patchmatch_update(gpointer app);

    PatchMatchApp();
    ~PatchMatchApp();
    void openFile(const char *filename);
    void saveFile();
    void saveFileAs(const char *filename);
    void updateTarget();
};

#endif
