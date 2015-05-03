#include "patchmatchapp.h"

PatchMatchApp::PatchMatchApp()
{
    GtkBuilder *builder;
    GError *error = NULL;
    // Open interface definition
    builder = gtk_builder_new_from_file("main_window.glade");
    // Get widgets
    main_window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    image = GTK_WIDGET(gtk_builder_get_object(builder, "image"));
    menu_file_open = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_open"));
    menu_file_save = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_save"));
    menu_file_save_as = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_save_as"));
    menu_file_quit = GTK_WIDGET(gtk_builder_get_object(builder, "menu_file_quit"));
    // Connect signals
    g_signal_connect(G_OBJECT(menu_file_open), "activate", G_CALLBACK(PatchMatchApp::cb_menu_file_open), this);
    g_signal_connect(G_OBJECT(menu_file_save), "activate", G_CALLBACK(PatchMatchApp::cb_menu_file_save), this);
    g_signal_connect(G_OBJECT(menu_file_save_as), "activate", G_CALLBACK(PatchMatchApp::cb_menu_file_save_as), this);
    g_signal_connect(G_OBJECT(menu_file_quit), "activate", G_CALLBACK(PatchMatchApp::cb_menu_file_quit), this);
    // Other initializations
    filename = NULL;
    
    gtk_widget_show_all(main_window);
    g_object_unref(G_OBJECT(builder));
}

PatchMatchApp::~PatchMatchApp()
{
    if(filename != NULL)
        delete filename;
}

void PatchMatchApp::cb_menu_file_open(GtkWidget* widget, gpointer *app)
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

void PatchMatchApp::cb_menu_file_save(GtkWidget* widget, gpointer *app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    self->saveFile();
}

void PatchMatchApp::cb_menu_file_save_as(GtkWidget* widget, gpointer *app)
{
    GtkWidget *dialog;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;

    PatchMatchApp *self = (PatchMatchApp*) app;
    dialog = gtk_file_chooser_dialog_new("Save File As", GTK_WINDOW(self->main_window), 
        action, "Cancel", GTK_RESPONSE_CANCEL, "Save As", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER(dialog), TRUE);
    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        char *filename;
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        self->saveFileAs(filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
}

void PatchMatchApp::cb_menu_file_quit(GtkWidget* widget, gpointer *app)
{
    PatchMatchApp *self = (PatchMatchApp*) app;
    gtk_widget_destroy(self->main_window);
    gtk_main_quit();
}

void PatchMatchApp::openFile(const char *filename)
{
    if(this->filename != NULL)
    {
        delete this->filename;
        this->filename = NULL;
    }
    int length = strlen(filename) + 1;
    this->filename = new char[length];
    strncpy(this->filename, filename, length);
}

void PatchMatchApp::saveFile()
{
    saveFileAs(this->filename);
}

void PatchMatchApp::saveFileAs(const char *filename)
{
}
