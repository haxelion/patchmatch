#include "patchmatchapp.h"

PatchMatchApp::PatchMatchApp()
{
    GtkBuilder *builder;

    // Open interface definition
    builder = gtk_builder_new_from_file("ui.glade");
    // Get widgets
    main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    drawing_area = GTK_WIDGET(gtk_builder_get_object(builder, "drawing_area"));
    menu_file_open = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_open"));
    menu_file_save = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_save"));
    menu_file_save_as = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_save_as"));
    menu_file_save_as = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_save_as"));
    menu_file_quit = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_quit"));
    menu_edit_settings = GTK_WIDGET(gtk_builder_get_object(builder, "menu_edit_settings"));
    menu_help_about = GTK_WIDGET(gtk_builder_get_object(builder, "menu_help_about"));
    tool_move = GTK_WIDGET(gtk_builder_get_object(builder, "tool_move"));
    tool_delete = GTK_WIDGET(gtk_builder_get_object(builder, "tool_delete"));
    tool_reshuffle_rectangle = GTK_WIDGET(gtk_builder_get_object(builder, "tool_reshuffle_rectangle"));
    tool_reshuffle_free = GTK_WIDGET(gtk_builder_get_object(builder, "tool_reshuffle_free"));
    tool_replace_rectangle = GTK_WIDGET(gtk_builder_get_object(builder, "tool_replace_rectangle"));
    tool_replace_free = GTK_WIDGET(gtk_builder_get_object(builder, "tool_replace_free"));
    tool_retarget = GTK_WIDGET(gtk_builder_get_object(builder, "tool_retarget"));
    tool_process = GTK_WIDGET(gtk_builder_get_object(builder, "tool_process"));
    gtk_widget_add_events(drawing_area, GDK_POINTER_MOTION_MASK | 
                          GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

    settings_dialog = GTK_WIDGET(gtk_builder_get_object(builder, "settings_dialog"));
    settings_apply = GTK_WIDGET(gtk_builder_get_object(builder, "settings_apply"));
    settings_cancel = GTK_WIDGET(gtk_builder_get_object(builder, "settings_cancel"));
    settings_patch_w = GTK_WIDGET(gtk_builder_get_object(builder, "settings_patch_w"));
    settings_pm_iter = GTK_WIDGET(gtk_builder_get_object(builder, "settings_pm_iter"));
    settings_em_iter = GTK_WIDGET(gtk_builder_get_object(builder, "settings_em_iter"));

    progress_window = GTK_WIDGET(gtk_builder_get_object(builder, "progress_window"));
    progress_bar = GTK_WIDGET(gtk_builder_get_object(builder, "progress_bar"));
    progress_cancel = GTK_WIDGET(gtk_builder_get_object(builder, "progress_cancel"));
    // Connect signals
    g_signal_connect(G_OBJECT(menu_file_open), "activate", G_CALLBACK(PatchMatchApp::cb_menu_file_open), this);
    g_signal_connect(G_OBJECT(menu_file_save), "activate", G_CALLBACK(PatchMatchApp::cb_menu_file_save), this);
    g_signal_connect(G_OBJECT(menu_file_save_as), "activate", G_CALLBACK(PatchMatchApp::cb_menu_file_save_as), this);
    g_signal_connect(G_OBJECT(menu_file_quit), "activate", G_CALLBACK(PatchMatchApp::cb_menu_file_quit), this);
    g_signal_connect(G_OBJECT(menu_edit_settings), "activate", G_CALLBACK(PatchMatchApp::cb_menu_edit_settings), this);
    g_signal_connect(G_OBJECT(menu_help_about), "activate", G_CALLBACK(PatchMatchApp::cb_menu_help_about), this);
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
    g_signal_connect(G_OBJECT(tool_replace_rectangle), "clicked", G_CALLBACK(PatchMatchApp::cb_toolbar_clicked), this);
    g_signal_connect(G_OBJECT(tool_replace_free), "clicked", G_CALLBACK(PatchMatchApp::cb_toolbar_clicked), this);
    g_signal_connect(G_OBJECT(tool_retarget), "clicked", G_CALLBACK(PatchMatchApp::cb_toolbar_clicked), this);

    g_signal_connect(G_OBJECT(settings_apply), "clicked", G_CALLBACK(PatchMatchApp::cb_settings_apply), this);
    g_signal_connect(G_OBJECT(settings_cancel), "clicked", G_CALLBACK(PatchMatchApp::cb_settings_canceled), this);
    g_signal_connect(G_OBJECT(settings_dialog), "delete-event", G_CALLBACK(PatchMatchApp::cb_settings_canceled), this);

    g_signal_connect(G_OBJECT(progress_window), "delete-event", G_CALLBACK(PatchMatchApp::cb_patchmatch_canceled), this);
    g_signal_connect(G_OBJECT(progress_cancel), "clicked", G_CALLBACK(PatchMatchApp::cb_patchmatch_canceled), this);

    // Other initializations
    filename = NULL;
    source = NULL;
    target = NULL;
    zones = new std::vector<Zone*>();
    button_pressed = false;
    active_tool = TOOL_NONE;
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
    for(unsigned int i = 0; i < zones->size(); i++)
        delete zones->at(i);
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
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pixbuf_formats(filter);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
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
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pixbuf_formats(filter);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), filter);
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

void PatchMatchApp::cb_menu_edit_settings(GtkWidget* widget, gpointer app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    gtk_widget_show_all(self->settings_dialog);
}

void PatchMatchApp::cb_menu_help_about(GtkWidget* widget, gpointer app)
{
}

void PatchMatchApp::cb_settings_apply(GtkWidget *widget, GdkEvent *event, gpointer app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    self->algo.patch_w = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(self->settings_patch_w));
    self->algo.patchmatch_iteration = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(self->settings_pm_iter));
    self->algo.em_iteration = (int) gtk_spin_button_get_value(GTK_SPIN_BUTTON(self->settings_em_iter));
    gtk_widget_hide(self->settings_dialog);
}

gboolean PatchMatchApp::cb_settings_canceled(GtkWidget *widget, GdkEvent *event, gpointer app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    gtk_widget_hide(self->settings_dialog);
    return TRUE;
}

gboolean PatchMatchApp::cb_draw(GtkWidget *widget, cairo_t *cr, gpointer app)
{
    guint width, height;

    PatchMatchApp *self = (PatchMatchApp*) app;
    if(self->target != NULL)
    {
        width = gtk_widget_get_allocated_width(widget);
        height = gtk_widget_get_allocated_height(widget);
        self->scale = fmin(width/(double) cairo_image_surface_get_width(self->target), 
                           height/(double) cairo_image_surface_get_height(self->target));
        cairo_scale(cr, self->scale, self->scale); 
        cairo_set_source_surface(cr, self->target, 0, 0);
        cairo_paint(cr);
        if(self->algo.isDone())
        {
            for(unsigned int i = 0; i < self->zones->size(); i++)
            {
                self->zones->at(i)->draw(cr, self->source, 1, true);
            }
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
                if(self->zones->at(i)->contains(e->x/self->scale, e->y/self->scale, 1))
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
                if(self->zones->at(i)->contains(e->x/self->scale, e->y/self->scale, 1))
                {
                    self->button_pressed = true;
                    self->move.zone = i;
                    self->move.dx = self->zones->at(i)->dst_x - e->x/self->scale;
                    self->move.dy = self->zones->at(i)->dst_y - e->y/self->scale;
                    break;
                }
            }
        }
        else if(self->active_tool == TOOL_RESHUFFLE_RECTANGLE)
        {
            self->button_pressed = true;
            self->zones->push_back(new Zone(e->x/self->scale, e->y/self->scale, FIXEDZONE));
            gtk_widget_queue_draw(self->drawing_area);
        }
        else if(self->active_tool == TOOL_RESHUFFLE_FREE_HAND)
        {
            self->button_pressed = true;
            self->zones->push_back(new MaskedZone(e->x/self->scale, e->y/self->scale, FIXEDZONE));
            gtk_widget_queue_draw(self->drawing_area);
        }
        else if(self->active_tool == TOOL_REPLACE_RECTANGLE)
        {
            self->button_pressed = true;
            self->zones->push_back(new Zone(e->x/self->scale, e->y/self->scale, REPLACEZONE));
            gtk_widget_queue_draw(self->drawing_area);
        }
        else if(self->active_tool == TOOL_REPLACE_FREE_HAND)
        {
            self->button_pressed = true;
            self->zones->push_back(new MaskedZone(e->x/self->scale, e->y/self->scale, REPLACEZONE));
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
        if((self->active_tool == TOOL_RESHUFFLE_RECTANGLE ||
            self->active_tool == TOOL_RESHUFFLE_FREE_HAND ||
            self->active_tool == TOOL_REPLACE_RECTANGLE || 
            self->active_tool == TOOL_REPLACE_FREE_HAND) && 
            self->zones->size() >= 0)
        {
            self->zones->back()->finalize();
        }
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
            self->zones->at(self->move.zone)->dst_x = e->x/self->scale + self->move.dx;
            self->zones->at(self->move.zone)->dst_y = e->y/self->scale + self->move.dy;
            gtk_widget_queue_draw(self->drawing_area);
        }
        else if((self->active_tool == TOOL_RESHUFFLE_RECTANGLE ||
            self->active_tool == TOOL_RESHUFFLE_FREE_HAND ||
            self->active_tool == TOOL_REPLACE_RECTANGLE || 
            self->active_tool == TOOL_REPLACE_FREE_HAND) && 
            self->zones->size() >= 0)
        {
            self->zones->back()->extend(e->x/self->scale, e->y/self->scale);
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
    else if(widget == self->tool_replace_rectangle)
    {
        self->active_tool = TOOL_REPLACE_RECTANGLE;
    }
    else if(widget == self->tool_replace_free)
    {
        self->active_tool = TOOL_REPLACE_FREE_HAND;
    }
    else if(widget == self->tool_retarget)
    {
        self->active_tool = TOOL_RETARGET;
    }
    else if(widget == self->tool_process)
    {
        if(self->source != NULL && self->target != NULL)
        {
            self->algo.run(self->source, self->target, self->zones);
            gtk_button_set_label(GTK_BUTTON(self->progress_cancel), "Cancel");
            gtk_button_set_relief(GTK_BUTTON(self->progress_cancel), GTK_RELIEF_NORMAL);
            gtk_widget_show_all(self->progress_window);
            g_idle_add(PatchMatchApp::cb_patchmatch_update, self);
        }
    }
 }

gboolean PatchMatchApp::cb_patchmatch_canceled(GtkWidget *widget, GdkEvent *event, gpointer app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    gtk_button_set_label(GTK_BUTTON(self->progress_cancel), "Canceling ...");
    gtk_button_set_relief(GTK_BUTTON(self->progress_cancel), GTK_RELIEF_NONE);
    self->algo.stop();
    return TRUE;
}

gboolean PatchMatchApp::cb_patchmatch_update(gpointer app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->progress_bar), self->algo.getProgress());
    if(self->algo.getResult() != NULL && self->target != self->algo.getResult())
    {
        if(self->target != NULL)
            cairo_surface_destroy(self->target);
        self->target = self->algo.getResult();
        cairo_surface_reference(self->target);
        gtk_widget_queue_draw(self->drawing_area);
    }
    if(self->algo.isDone())
    {
        if(self->algo.terminate)
        {
            cairo_surface_destroy(self->target);
            self->target = cairo_image_surface_create(
                CAIRO_FORMAT_RGB24, 
                cairo_image_surface_get_width(self->source), 
                cairo_image_surface_get_height(self->source));
            cairo_t *cr = cairo_create(self->target);
            cairo_set_source_surface(cr, self->source, 0, 0);
            cairo_paint(cr);
            cairo_surface_flush(self->target);
            cairo_destroy(cr);
        }
        else
        {
            cairo_surface_destroy(self->source);
            self->source = cairo_image_surface_create(
                CAIRO_FORMAT_RGB24, 
                cairo_image_surface_get_width(self->target), 
                cairo_image_surface_get_height(self->target));
            cairo_t *cr = cairo_create(self->source);
            cairo_set_source_surface(cr, self->target, 0, 0);
            cairo_paint(cr);
            cairo_surface_flush(self->source);
            cairo_destroy(cr);
        }
        gtk_widget_hide(self->progress_window);
        return FALSE;
    }
    return TRUE;
}

void PatchMatchApp::openFile(const char *filename)
{
    GError *error = NULL;
    GdkPixbuf *pixbuf;
    cairo_t *cr;

    if(this->filename != NULL)
    {
        delete this->filename;
        this->filename = NULL;
    }
    if(source != NULL)
    {
        cairo_surface_destroy(source);
        source = NULL;
    }
    if(target != NULL)
    {
        cairo_surface_destroy(target);
        target = NULL;
    }
    pixbuf = gdk_pixbuf_new_from_file(filename, &error);
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
    source = cairo_image_surface_create(
        CAIRO_FORMAT_RGB24, 
        gdk_pixbuf_get_width(pixbuf), 
        gdk_pixbuf_get_height(pixbuf));
    cr = cairo_create(source);
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);
    cairo_surface_flush(source);
    cairo_destroy(cr);

    target = cairo_image_surface_create(
        CAIRO_FORMAT_RGB24, 
        gdk_pixbuf_get_width(pixbuf), 
        gdk_pixbuf_get_height(pixbuf));
    cr = cairo_create(target);
    gdk_cairo_set_source_pixbuf(cr, pixbuf, 0, 0);
    cairo_paint(cr);
    cairo_surface_flush(target);
    cairo_destroy(cr);

    g_object_unref(pixbuf);
     
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
    GError *error = NULL;
    GdkPixbuf *pixbuf;
    const char *type = strrchr(filename, '.');
    if(type == NULL)
    {
        GtkWidget *dialog;
        GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;

        dialog = gtk_message_dialog_new(GTK_WINDOW(this->main_window), flags, GTK_MESSAGE_ERROR, 
            GTK_BUTTONS_CLOSE, "Missing file extension.");
        gtk_dialog_run(GTK_DIALOG (dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    type += 1;
    pixbuf = gdk_pixbuf_get_from_surface(target, 0, 0,
        cairo_image_surface_get_width(target),
        cairo_image_surface_get_height(target));
    gdk_pixbuf_save(pixbuf, filename, type, &error, NULL);
    g_object_unref(pixbuf);
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
}


