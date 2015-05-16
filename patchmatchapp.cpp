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
    tool_move = GTK_WIDGET(gtk_builder_get_object(builder, "tool_move"));
    tool_delete = GTK_WIDGET(gtk_builder_get_object(builder, "tool_delete"));
    tool_reshuffle_rectangle = GTK_WIDGET(gtk_builder_get_object(builder, "tool_reshuffle_rectangle"));
    tool_reshuffle_free = GTK_WIDGET(gtk_builder_get_object(builder, "tool_reshuffle_free"));
    tool_process = GTK_WIDGET(gtk_builder_get_object(builder, "tool_process"));
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
    g_signal_connect(G_OBJECT(tool_move), "clicked", G_CALLBACK(PatchMatchApp::cb_toolbar_clicked), this);
    g_signal_connect(G_OBJECT(tool_delete), "clicked", G_CALLBACK(PatchMatchApp::cb_toolbar_clicked), this);
    g_signal_connect(G_OBJECT(tool_reshuffle_rectangle), "clicked", G_CALLBACK(PatchMatchApp::cb_toolbar_clicked), this);
    g_signal_connect(G_OBJECT(tool_reshuffle_free), "clicked", G_CALLBACK(PatchMatchApp::cb_toolbar_clicked), this);
    g_signal_connect(G_OBJECT(tool_process), "clicked", G_CALLBACK(PatchMatchApp::cb_toolbar_clicked), this);
    // Other initializations
    filename = NULL;
    source = NULL;
    target = NULL;
    zones = new std::vector<Zone>();
    button_pressed = false;
    active_tool = TOOL_NONE;
    algo = new PatchMatchAlgo();
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
    delete zones;
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
        self->scale = fmin(width/(double) gdk_pixbuf_get_width(self->target), 
                           height/(double) gdk_pixbuf_get_height(self->target));
        cairo_scale(cr, self->scale, self->scale); 
        gdk_cairo_set_source_pixbuf(cr, self->target, 0, 0);
        cairo_paint(cr);
        for(unsigned int i = 0; i < self->zones->size(); i++)
        {
            cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
            cairo_set_line_width(cr, 3.0);
            cairo_rectangle(cr, 
                            self->zones->at(i).dst_x, 
                            self->zones->at(i).dst_y,
                            self->zones->at(i).src_width, 
                            self->zones->at(i).src_height);
            cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
            cairo_set_line_width(cr, 3.0);
            cairo_stroke_preserve(cr);
            cairo_clip(cr);
            gdk_cairo_set_source_pixbuf(cr, self->target,
                self->zones->at(i).dst_x - self->zones->at(i).src_x, 
                self->zones->at(i).dst_y - self->zones->at(i).src_y);
            cairo_paint(cr);
            cairo_reset_clip(cr);
        }
    }
    return FALSE;
}

void PatchMatchApp::cb_button_pressed(GtkWidget *widget, GdkEvent *event, gpointer app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    GdkEventButton *e = (GdkEventButton*) event;
    if(e->button == 1)
    {
        if(self->active_tool == TOOL_DELETE)
        {
            for(unsigned int i = 0; i < self->zones->size(); i++)
            {
                if(self->zones->at(i).contains(e->x/self->scale, e->y/self->scale))
                {
                    self->zones->erase(self->zones->begin() + i);
                    gtk_widget_queue_draw(self->drawing_area);
                }
            }
        }
        else if(self->active_tool == TOOL_MOVE)
        {
            for(int i = self->zones->size() - 1; i >= 0; i--)
            {
                if(self->zones->at(i).contains(e->x/self->scale, e->y/self->scale))
                {
                    self->button_pressed = true;
                    self->move.zone = i;
                    self->move.dx = self->zones->at(i).dst_x - e->x/self->scale;
                    self->move.dy = self->zones->at(i).dst_y - e->y/self->scale;
                    break;
                }
            }
        }
        else if(self->active_tool == TOOL_RESHUFFLE_RECTANGLE)
        {
            self->button_pressed = true;
            self->zones->push_back(Zone(e->x/self->scale, e->y/self->scale, FIXEDZONE));
            gtk_widget_queue_draw(self->drawing_area);
        }
    }
}

void PatchMatchApp::cb_button_released(GtkWidget *widget, GdkEvent *event, gpointer app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    GdkEventButton *e = (GdkEventButton*) event;
    if(e->button == 1 && self->button_pressed == true)
    {
        self->button_pressed = false;
        if(self->active_tool == TOOL_RESHUFFLE_RECTANGLE && 
           self->zones->size() >= 0)
            self->zones->back().finalize();
    }
}

void PatchMatchApp::cb_motion_notify(GtkWidget *widget, GdkEvent *event, gpointer app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    GdkEventMotion *e = (GdkEventMotion*) event;
    if(self->button_pressed)
    {
        if(self->active_tool == TOOL_MOVE)
        {
            self->zones->at(self->move.zone).dst_x = e->x/self->scale + self->move.dx;
            self->zones->at(self->move.zone).dst_y = e->y/self->scale + self->move.dy;
            gtk_widget_queue_draw(self->drawing_area);
        }
        else if(self->active_tool == TOOL_RESHUFFLE_RECTANGLE && 
           self->zones->size() >= 0)
        {
            self->zones->back().extend(e->x/self->scale, e->y/self->scale);
            gtk_widget_queue_draw(self->drawing_area);
        }
    }
}

void PatchMatchApp::cb_toolbar_clicked(GtkWidget* widget, gpointer app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    if(widget == self->tool_move)
    {
        self->active_tool = TOOL_MOVE;
    }
    else if(widget == self->tool_delete)
    {
        self->active_tool = TOOL_DELETE;
    }
    else if(widget == self->tool_reshuffle_rectangle)
    {
        self->active_tool = TOOL_RESHUFFLE_RECTANGLE;
    }
    else if(widget == self->tool_reshuffle_free)
    {
        self->active_tool = TOOL_RESHUFFLE_FREE_HAND;
    }
    else if(widget == self->tool_process)
    {
        self->algo->run(self->source, self->target, self->zones);
        self->progress_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(self->progress_window), "Working");
        gtk_window_set_resizable(GTK_WINDOW(self->progress_window), FALSE);
        gtk_window_set_modal(GTK_WINDOW(self->progress_window), TRUE);
        gtk_window_set_transient_for(GTK_WINDOW(self->progress_window), GTK_WINDOW(self->main_window));
        gtk_window_set_position(GTK_WINDOW(self->progress_window), GTK_WIN_POS_CENTER_ON_PARENT);
        self->level_bar = gtk_level_bar_new();
        gtk_widget_set_size_request(self->level_bar, 200, 50);
        gtk_container_add(GTK_CONTAINER(self->progress_window), self->level_bar);
        gtk_widget_show_all(self->progress_window);
        g_idle_add(PatchMatchApp::cb_patchmatch_update, self);
    }
 }

gboolean PatchMatchApp::cb_patchmatch_update(gpointer app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    gtk_level_bar_set_value(GTK_LEVEL_BAR(self->level_bar), self->algo->getProgress());
    if(self->algo->getResult() != NULL && self->target != self->algo->getResult())
    {
        if(self->target != NULL)
            g_object_unref(self->target);
        self->target = self->algo->getResult();
        g_object_ref(self->target);
        gtk_widget_queue_draw(self->drawing_area);
    }
    if(self->algo->isDone())
    {
        gtk_widget_destroy(self->progress_window);
        return FALSE;
    }
    return TRUE;
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
    if(target != NULL)
    {
        g_object_unref(target);
        target = NULL;
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
    target = gdk_pixbuf_copy(source);
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


