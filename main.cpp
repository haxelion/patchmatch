#include <gtk/gtk.h>
#include <stdio.h>
#include "patchmatchapp.h"

int main(int argc, char **argv)
{
    PatchMatchApp *patchmatchapp;
    gtk_init(&argc, &argv);
    patchmatchapp = new PatchMatchApp();
    gtk_main();
    delete patchmatchapp;
    return 0;
}
