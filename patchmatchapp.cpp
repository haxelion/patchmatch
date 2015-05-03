#include "patchmatchapp.h"

PatchMatchApp::PatchMatchApp()
{
    GtkBuilder *builder;

    // Open interface definition
    builder = gtk_builder_new_from_file("main_window.glade");
    // Get widgets
    main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    drawing_area = GTK_WIDGET(gtk_builder_get_object(builder, "drawing_area"));
    menu_file_open = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_open"));
    menu_file_save = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_save"));
    menu_file_save_as = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_save_as"));
    menu_file_quit = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_quit"));
    gtk_widget_add_events(drawing_area, GDK_POINTER_MOTION_MASK | 
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
    // Connect signals
    g_signal_connect(G_OBJECT(menu_file_open), "activate", G_CALLBACK(PatchMatchApp::cb_menu_file_open), this);
    g_signal_connect(G_OBJECT(menu_file_save), "activate", G_CALLBACK(PatchMatchApp::cb_menu_file_save), this);
    g_signal_connect(G_OBJECT(menu_file_save_as), "activate", G_CALLBACK(PatchMatchApp::cb_menu_file_save_as), this);
    g_signal_connect(G_OBJECT(menu_file_quit), "activate", G_CALLBACK(PatchMatchApp::cb_menu_file_quit), this);
    g_signal_connect(G_OBJECT(main_window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(PatchMatchApp::cb_draw), this);
    g_signal_connect(G_OBJECT(drawing_area), "button-press-event", G_CALLBACK(PatchMatchApp::cb_button_pressed), this);
    g_signal_connect(G_OBJECT(drawing_area), "button-release-event", G_CALLBACK(PatchMatchApp::cb_button_released), this);
    g_signal_connect(G_OBJECT(drawing_area), "motion-notify-event", G_CALLBACK(PatchMatchApp::cb_motion_notify), this);
    // Other initializations
    filename = NULL;
    source = NULL;
    target = NULL;
    fixed_zones = new std::vector<FixedZone>();
    // Lock & Load
    gtk_widget_show_all(main_window);
    g_object_unref(G_OBJECT(builder));
}

PatchMatchApp::~PatchMatchApp()
{
    if(filename != NULL)
        delete filename;
    if(target != NULL)
        g_object_unref(target);
    if(source != NULL)
        g_object_unref(source);
    delete fixed_zones;
}

void PatchMatchApp::cb_menu_file_open(GtkWidget* widget, gpointer app)
{
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    PatchMatchApp *self = (PatchMatchApp*) app;
    dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(self->main_window), 
        action, "Cancel", GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_ACCEPT, NULL);
    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        self->openFile(filename);
        g_free(filename);
    }

    gtk_widget_destroy (dialog);
}

void PatchMatchApp::cb_menu_file_save(GtkWidget* widget, gpointer app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    self->saveFile();
}

void PatchMatchApp::cb_menu_file_save_as(GtkWidget* widget, gpointer app)
{
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;

    PatchMatchApp *self = (PatchMatchApp*) app;
    dialog = gtk_file_chooser_dialog_new("Save File As", GTK_WINDOW(self->main_window), 
        action, "Cancel", GTK_RESPONSE_CANCEL, "Save As", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER(dialog), TRUE);
    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if(res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        self->saveFileAs(filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void PatchMatchApp::cb_menu_file_quit(GtkWidget* widget, gpointer app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    gtk_widget_destroy(self->main_window);
}

gboolean PatchMatchApp::cb_draw(GtkWidget *widget, cairo_t *cr, gpointer app)
{
    guint width, height;

    PatchMatchApp *self = (PatchMatchApp*) app;
    if(self->target != NULL)
    {
        width = gtk_widget_get_allocated_width(widget);
        height = gtk_widget_get_allocated_height(widget);
        double scale = fmin(width/(double) gdk_pixbuf_get_width(self->target), 
                           height/(double) gdk_pixbuf_get_height(self->target));
        cairo_scale(cr, scale, scale); 
        gdk_cairo_set_source_pixbuf(cr, self->target, 0, 0);
        cairo_paint(cr);
    }
    return FALSE;
}

void PatchMatchApp::cb_button_pressed(GtkWidget *widget, GdkEvent *event, gpointer app)
{
    GdkEventButton *e = (GdkEventButton*) event;
    printf("Button pressed at (%lf,%lf)\n", e->x, e->y);
}

void PatchMatchApp::cb_button_released(GtkWidget *widget, GdkEvent *event, gpointer app)
{
    GdkEventButton *e = (GdkEventButton*) event;
    printf("Button released at (%lf,%lf)\n", e->x, e->y);
}

void PatchMatchApp::cb_motion_notify(GtkWidget *widget, GdkEvent *event, gpointer app)
{
    GdkEventMotion *e = (GdkEventMotion*) event;
    printf("Mouse moved at (%lf,%lf)\n", e->x, e->y);
}

void PatchMatchApp::openFile(const char *filename)
{
    GError *error = NULL;

    if(this->filename != NULL)
    {
        delete this->filename;
        this->filename = NULL;
    }
    if(source != NULL)
    {
        g_object_unref(source);
        source = NULL;
    }
    source = gdk_pixbuf_new_from_file(filename, &error);
    if(error != NULL)
    {
        GtkWidget *dialog;
        GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;

        dialog = gtk_message_dialog_new(GTK_WINDOW(this->main_window), flags, GTK_MESSAGE_ERROR, 
            GTK_BUTTONS_CLOSE, "%s", error->message);
        gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        return;
    }
    int length = strlen(filename) + 1;
    this->filename = new char[length];
    strncpy(this->filename, filename, length);
    gtk_widget_queue_draw(drawing_area);
}

void PatchMatchApp::saveFile()
{
    saveFileAs(this->filename);
}

void PatchMatchApp::saveFileAs(const char *filename)
{
}
