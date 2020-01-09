#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "gtkiconfilesel.h"
#include "sg_small.xpm"

GtkWidget *filesel;
GtkWidget *show_button;
GtkWidget *hide_button;


void
quit ()
{
  gtk_main_quit();
}

void
ok_clicked(GtkWidget *widget, gpointer data)
{
  GtkIconFileSel *filesel;
  const gchar *path;
  const gchar *file;
  const gchar *selection;

  filesel = GTK_ICON_FILESEL(data);
  path = gtk_file_list_get_path(GTK_FILE_LIST(filesel->file_list));
  file = gtk_file_list_get_filename(GTK_FILE_LIST(filesel->file_list));
  selection = gtk_icon_file_selection_get_selection(filesel);

  if(path)
      printf("PATH: %s\n",path); 
  if(file)
      printf("FILE: %s\n",file); 
  if(selection)
      printf("SELECTION: %s\n",selection);

}

static void
show_tree(GtkWidget *widget, gpointer data)
{
  gtk_icon_file_selection_show_tree(GTK_ICON_FILESEL(filesel), TRUE); 
}

static void
hide_tree(GtkWidget *widget, gpointer data)
{
  gtk_icon_file_selection_show_tree(GTK_ICON_FILESEL(filesel), FALSE); 
}

int main(int argc, char *argv[]) 
{
  GtkWidget *box;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GdkColormap *colormap;
  gint type;

  gtk_init(&argc, &argv);

  colormap = gdk_colormap_get_system();

  filesel=gtk_icon_file_selection_new("Open File");

  pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
                                                 sg_small_xpm);

  type = gtk_file_list_add_type_with_pixmap
                        (GTK_FILE_LIST(GTK_ICON_FILESEL(filesel)->file_list),
                         pixmap, mask);
  gtk_file_list_add_type_filter
                        (GTK_FILE_LIST(GTK_ICON_FILESEL(filesel)->file_list),
                         type,
                         "*.sg");
  gtk_file_list_add_type_filter
                        (GTK_FILE_LIST(GTK_ICON_FILESEL(filesel)->file_list),
                         type,
                         "*.sgp");
  gtk_file_list_add_type_filter
                        (GTK_FILE_LIST(GTK_ICON_FILESEL(filesel)->file_list),
                         type,
                         "*.sgw");

  box = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(GTK_BIN(filesel)->child), box, TRUE, TRUE, 0);
  gtk_widget_show(box);

  show_button = gtk_button_new_with_label("Show Tree");
  gtk_box_pack_start(GTK_BOX(box), show_button, TRUE, TRUE, 0);
  gtk_widget_show(show_button);

  hide_button = gtk_button_new_with_label("Hide Tree");
  gtk_box_pack_start(GTK_BOX(box), hide_button, TRUE, TRUE, 0);
  gtk_widget_show(hide_button);

  gtk_signal_connect (GTK_OBJECT (filesel), "destroy",
		      GTK_SIGNAL_FUNC (quit), NULL);

  gtk_signal_connect (GTK_OBJECT (GTK_ICON_FILESEL(filesel)->ok_button), 
                     "clicked",
		      GTK_SIGNAL_FUNC (ok_clicked), filesel);

  gtk_signal_connect (GTK_OBJECT (show_button), "clicked",
		      GTK_SIGNAL_FUNC (show_tree), NULL);

  gtk_signal_connect (GTK_OBJECT (hide_button), "clicked",
		      GTK_SIGNAL_FUNC (hide_tree), NULL);

  gtk_widget_show(filesel);
  gtk_main();

  return(0);
}


