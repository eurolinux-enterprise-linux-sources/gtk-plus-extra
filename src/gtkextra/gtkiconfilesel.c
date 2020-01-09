/* gtkiconfileselection - gtkiconfileselection dialog widget for gtk+
 * Copyright 1999-2001  Adrian E. Feiguin <feiguin@ifir.edu.ar>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "gtkiconfilesel.h"
#include "gtkextraicons.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

static void gtk_icon_file_selection_class_init          (GtkIconFileSelClass *klass);
static void gtk_icon_file_selection_init                (GtkIconFileSel *filesel);
static void gtk_icon_file_selection_destroy             (GtkObject *object);
static void open_dir					(GtkWidget *widget, 
							 GtkCTreeNode *node, 
							 gint n,
                                                         gpointer data);
static gboolean entry_set_file				(GtkWidget *widget, 
							 GdkEventKey *key, 
							 gpointer data);
static void real_set_file				(GtkWidget *widget, 
							 gpointer data);
static gboolean set_filter				(GtkWidget *widget, 
                                                         GdkEventKey *key,
							 gpointer data);
static gboolean select_icon				(GtkIconList *iconlist, 
            						 GtkIconListItem *icon,
            						 GdkEvent *event, 
							 gpointer data);
static gboolean insert_text                             (GtkEditable *editable, 
                                                         const gchar *new_text,
                                                         gint  new_text_length, 
                                                         gint  *position,
                                                         gpointer data);
static void init_history_combo				(GtkIconFileSel *filesel, 
							 const gchar *curr_dir);
static void update_history_combo			(GtkIconFileSel *filesel, 
							 const gchar *curr_dir);
static void go_to_history				(GtkEntry *entry,
							 gpointer data);
static gboolean combo_changed				(GtkWidget *widget, 
							 GtkWidget *child,
							 gpointer data);
static gboolean entry_key_press				(GtkWidget *widget, 
							 GdkEventKey *event, 
							 gpointer data);
static gchar *get_real_path				(const gchar *full_path);

static GtkWindowClass *parent_class = NULL;


GtkType
gtk_icon_file_selection_get_type (void)
{
  static GtkType filesel_type = 0;
  
  if (!filesel_type)
    {
      GtkTypeInfo filesel_info =
      {
	"GtkIconFileSel",
	sizeof (GtkIconFileSel),
	sizeof (GtkIconFileSelClass),
	(GtkClassInitFunc) gtk_icon_file_selection_class_init,
	(GtkObjectInitFunc) gtk_icon_file_selection_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      
      filesel_type = gtk_type_unique (gtk_window_get_type(), &filesel_info);
    }
  
  return filesel_type;
}

GtkWidget*
gtk_icon_file_selection_new (const gchar *title)
{
  GtkWidget *widget;

  widget = gtk_widget_new (gtk_icon_file_selection_get_type(), NULL);

  gtk_icon_file_selection_construct(GTK_ICON_FILESEL(widget), title);

  return widget;
}

void
gtk_icon_file_selection_construct (GtkIconFileSel *file_sel, const gchar *title)
{
/*  GTK_ICON_FILESEL(widget)->title = g_strdup(title);
*/
  gtk_window_set_title(GTK_WINDOW(file_sel),title);
}

static void
gtk_icon_file_selection_class_init (GtkIconFileSelClass *klass)
{
  GtkWidgetClass *widget_class;
  GtkObjectClass *object_class;

  widget_class = (GtkWidgetClass*) klass;
  object_class = (GtkObjectClass*) klass;
  parent_class = gtk_type_class (gtk_window_get_type ());

  object_class->destroy = gtk_icon_file_selection_destroy;
}

static void
up_clicked(GtkWidget *widget, gpointer data)
{
  GtkIconFileSel *filesel = GTK_ICON_FILESEL(widget);
  gchar *current_dir;
  gint dir_len;
  gint i;

  current_dir = g_strdup (GTK_FILE_LIST(filesel->file_list)->path);
  dir_len = strlen (current_dir);

  for (i = dir_len - 1; i >= 0; i--){

    /* the i == dir_len is to catch the full path for the first
     * entry. */
    if ( current_dir[i] == G_DIR_SEPARATOR ) {
          current_dir[i + 1] = '\0';
          gtk_icon_file_selection_open_dir(filesel, current_dir);
          break;
     }
  }

  g_free(current_dir);
}

static void
refresh_clicked(GtkWidget *widget, gpointer data)
{
  GtkIconFileSel *filesel = GTK_ICON_FILESEL(widget);
  gtk_icon_file_selection_open_dir(filesel, GTK_FILE_LIST(filesel->file_list)->path);
}

static void
home_clicked(GtkWidget *widget, gpointer data)
{
  GtkIconFileSel *filesel = GTK_ICON_FILESEL(widget);
  gtk_icon_file_selection_open_dir(filesel, g_get_home_dir());
}


static void
gtk_icon_file_selection_init (GtkIconFileSel *filesel)
{
  GtkWidget *main_vbox;
  GtkWidget *hbox, *box;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *scrolled_window;
  GtkWidget *wpixmap;
  GtkTooltips *tp;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  gchar cwd_path[2*MAXPATHLEN] = "";
  gchar path[2*MAXPATHLEN] = "";
  GdkColormap *colormap = gtk_widget_get_colormap(GTK_WIDGET(filesel));

  filesel->show_tree = FALSE;

  /* We don't use getcwd() on SUNOS, because, it does a popen("pwd")
   * and, if that wasn't bad enough, hangs in doing so.
   */
#if defined(sun) && !defined(__SVR4)
  getwd (cwd_path);
#else
  getcwd (cwd_path, MAXPATHLEN);
#endif
  g_snprintf(path, MAXPATHLEN, "%s%s", cwd_path, G_DIR_SEPARATOR_S);

  gtk_window_set_policy(GTK_WINDOW(filesel), FALSE, FALSE, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (filesel), 10);

  main_vbox=gtk_vbox_new(FALSE,1);
  gtk_container_set_border_width(GTK_CONTAINER(main_vbox),0);
  gtk_container_add(GTK_CONTAINER(filesel), main_vbox);
  gtk_widget_show(main_vbox);

  hbox = gtk_hbox_new(FALSE, 1);
  gtk_box_pack_start(GTK_BOX(main_vbox), hbox, FALSE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new ("Go to:  "), FALSE, FALSE, 0);
  filesel->history_combo = gtk_combo_new();
  gtk_box_pack_start(GTK_BOX(hbox), filesel->history_combo, TRUE, TRUE, 0);
  gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(filesel->history_combo)->entry),
			 TRUE);
  gtk_signal_handler_block (GTK_OBJECT (GTK_COMBO(filesel->history_combo)->entry), GTK_COMBO(filesel->history_combo)->entry_change_id);
  init_history_combo(filesel, path);
  gtk_widget_show_all(hbox);

  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(filesel->history_combo)->entry), 
		     "key_press_event",
                     GTK_SIGNAL_FUNC(entry_key_press), filesel);

  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(filesel->history_combo)->list), 
		     "select_child",
                     GTK_SIGNAL_FUNC(combo_changed), filesel);

  filesel->up_button = gtk_button_new();
  pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
                                                 up_xpm);
  wpixmap = gtk_pixmap_new(pixmap, mask);
  gdk_pixmap_unref(pixmap);
  gdk_bitmap_unref(mask);

  gtk_container_add(GTK_CONTAINER(filesel->up_button), wpixmap);
  gtk_box_pack_start(GTK_BOX(hbox), filesel->up_button, FALSE, FALSE, 0);
  gtk_widget_show_all(filesel->up_button);
  gtk_signal_connect_object(GTK_OBJECT(filesel->up_button), "clicked",
		            GTK_SIGNAL_FUNC(up_clicked), GTK_OBJECT(filesel));
  tp = gtk_tooltips_new();
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tp), filesel->up_button, "Parent directory", "Parent directory");
  gtk_tooltips_enable(GTK_TOOLTIPS(tp));

  filesel->home_button = gtk_button_new();
  pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
                                                 home_xpm);
  wpixmap = gtk_pixmap_new(pixmap, mask);
  gdk_pixmap_unref(pixmap);
  gdk_bitmap_unref(mask);

  gtk_container_add(GTK_CONTAINER(filesel->home_button), wpixmap);
  gtk_box_pack_start(GTK_BOX(hbox), filesel->home_button, FALSE, FALSE, 0);
  gtk_widget_show_all(filesel->home_button);
  gtk_signal_connect_object(GTK_OBJECT(filesel->home_button), "clicked",
		            GTK_SIGNAL_FUNC(home_clicked), GTK_OBJECT(filesel));
  tp = gtk_tooltips_new();
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tp), filesel->home_button, "Home", "Home");
  gtk_tooltips_enable(GTK_TOOLTIPS(tp));

  filesel->refresh_button = gtk_button_new();
  pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
                                                 refresh_xpm);
  wpixmap = gtk_pixmap_new(pixmap, mask);
  gdk_pixmap_unref(pixmap);
  gdk_bitmap_unref(mask);

  gtk_container_add(GTK_CONTAINER(filesel->refresh_button), wpixmap);
  gtk_box_pack_start(GTK_BOX(hbox), filesel->refresh_button, FALSE, FALSE, 0);
  gtk_widget_show_all(filesel->refresh_button);
  gtk_signal_connect_object(GTK_OBJECT(filesel->refresh_button), "clicked",
		            GTK_SIGNAL_FUNC(refresh_clicked), GTK_OBJECT(filesel));
  tp = gtk_tooltips_new();
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tp), filesel->refresh_button, "Refresh", "Refresh");
  gtk_tooltips_enable(GTK_TOOLTIPS(tp));

  filesel->path_label = gtk_label_new(path);
  gtk_misc_set_alignment(GTK_MISC(filesel->path_label), 0., .5);
  gtk_box_pack_start(GTK_BOX(main_vbox), filesel->path_label, FALSE, TRUE, 0);
  gtk_widget_show(filesel->path_label);

  hbox=gtk_hbox_new(FALSE,1);
  gtk_box_pack_start(GTK_BOX(main_vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);

  filesel->tree_window = scrolled_window=gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_usize(scrolled_window, 200, 250);
  gtk_box_pack_start(GTK_BOX(hbox), scrolled_window, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);

/* FIXME
  filesel->dir_tree = gtk_dir_tree_new();
  GTK_DIR_TREE(filesel->dir_tree)->show_hidden = TRUE;
  gtk_container_add(GTK_CONTAINER(scrolled_window), filesel->dir_tree);
  gtk_widget_show(filesel->dir_tree);

  gtk_box_pack_start(GTK_BOX(hbox), gtk_vseparator_new(), TRUE, TRUE, 0);
*/

  filesel->list_window = scrolled_window=gtk_scrolled_window_new(NULL, NULL);
  gtk_box_pack_start(GTK_BOX(hbox), scrolled_window, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_ALWAYS,
                                 GTK_POLICY_AUTOMATIC);

  filesel->file_list = gtk_file_list_new(20, GTK_ICON_LIST_TEXT_RIGHT, G_DIR_SEPARATOR_S);
  GTK_ICON_LIST(filesel->file_list)->is_editable = FALSE;
  GTK_FILE_LIST(filesel->file_list)->show_folders = TRUE;
  GTK_FILE_LIST(filesel->file_list)->show_hidden = TRUE;
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), 
                                        filesel->file_list);
  gtk_widget_show(filesel->file_list);

  if(filesel->show_tree){ 
     gtk_icon_file_selection_show_tree(filesel, TRUE);
     gtk_widget_set_usize(filesel->list_window, 380, 250);
  }else{
     gtk_widget_set_usize(filesel->list_window, 550, 250);
  }

  gtk_widget_show(scrolled_window);

  gtk_signal_connect(GTK_OBJECT(filesel->file_list), "select_icon",
                     GTK_SIGNAL_FUNC(select_icon), filesel);

  filesel->action_area = table = gtk_table_new(TRUE, 2, 4);
  gtk_box_pack_start(GTK_BOX(main_vbox), table, TRUE, TRUE, 3);
  gtk_widget_show(table);

  label = gtk_label_new("File:        ");
  gtk_misc_set_alignment(GTK_MISC(label), 1., 0.5);
  gtk_table_attach_defaults(GTK_TABLE(table),
                            label,
                            0, 1, 0, 1);
  gtk_widget_show(label);

  label = gtk_label_new("Filter:        ");
  gtk_misc_set_alignment(GTK_MISC(label), 1., 0.5);
  gtk_table_attach_defaults(GTK_TABLE(table),
                            label,
                            0, 1, 1, 2);
  gtk_widget_show(label);

  filesel->file_entry = gtk_entry_new();
  gtk_table_attach_defaults(GTK_TABLE(table), filesel->file_entry, 1, 3, 0, 1);
  gtk_widget_show(filesel->file_entry);

  gtk_signal_connect(GTK_OBJECT(filesel->file_entry), "key_press_event",
                     GTK_SIGNAL_FUNC(entry_set_file), filesel);

  filesel->filter_entry = gtk_entry_new();
  gtk_table_attach_defaults(GTK_TABLE(table), filesel->filter_entry, 1, 3, 1, 2);
  gtk_widget_show(filesel->filter_entry);

  gtk_signal_connect(GTK_OBJECT(filesel->filter_entry), "key_press_event",
                     GTK_SIGNAL_FUNC(set_filter), filesel);

/* FIXME
  gtk_signal_connect(GTK_OBJECT(filesel->filter_entry), "insert_text",
                     GTK_SIGNAL_FUNC(insert_text), NULL);
*/

  box = gtk_vbutton_box_new();
  gtk_table_attach_defaults(GTK_TABLE(table), box, 3, 4, 0, 2);
  gtk_widget_show(box);

  pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
                                                 ok_xpm);
  wpixmap = gtk_pixmap_new(pixmap, mask);
  gdk_pixmap_unref(pixmap);
  gdk_bitmap_unref(mask);

  filesel->ok_button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_box_pack_end (GTK_BOX (box), filesel->ok_button, TRUE, TRUE, 0);
  gtk_widget_show(filesel->ok_button);

  gtk_signal_connect(GTK_OBJECT(filesel->ok_button), "clicked",
                     GTK_SIGNAL_FUNC(real_set_file), filesel);

  pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
                                                 cancel_xpm);
  wpixmap = gtk_pixmap_new(pixmap, mask);
  gdk_pixmap_unref(pixmap);
  gdk_bitmap_unref(mask);

  filesel->cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
  gtk_box_pack_end (GTK_BOX (box), filesel->cancel_button, TRUE, TRUE, 0);
  gtk_widget_show(filesel->cancel_button);

  gtk_icon_file_selection_open_dir(filesel, path);

  filesel->selection = NULL;
}

static void
gtk_icon_file_selection_destroy(GtkObject *object)
{
  if(GTK_ICON_FILESEL(object)->selection)
    g_free(GTK_ICON_FILESEL(object)->selection);
  GTK_ICON_FILESEL(object)->selection = NULL;

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

void
gtk_icon_file_selection_show_tree(GtkIconFileSel *filesel, gboolean show)
{
/* FIXME
  if(show == filesel->show_tree) return;

  filesel->show_tree = show;

  if(show){
    const gchar *path;

    filesel->tree_signal_id = gtk_signal_connect(GTK_OBJECT(filesel->dir_tree), 
                                                 "tree_select_row",
                                                 GTK_SIGNAL_FUNC(open_dir), 
                                                 filesel);

    path = gtk_file_list_get_path(GTK_FILE_LIST(filesel->file_list));
    gtk_dir_tree_open_dir(GTK_DIR_TREE(filesel->dir_tree), path);

    gtk_widget_set_usize(filesel->list_window, 380, 250);
    gtk_widget_show(filesel->tree_window);
  } else {
    gtk_signal_disconnect(GTK_OBJECT(filesel->dir_tree), 
                          filesel->tree_signal_id);
    gtk_widget_hide(filesel->tree_window);
    gtk_widget_set_usize(filesel->list_window, 550, 250);
  } 
*/
}

static gboolean
insert_text     (GtkEditable *editable,
                 const gchar *new_text,
                 gint         new_text_length,
                 gint        *position,
                 gpointer data)
{
  g_object_ref(editable);
  gtk_signal_emit_stop_by_name(GTK_OBJECT(editable), "insert_text");
  if(new_text[0] != ' '){
     GTK_EDITABLE_CLASS (gtk_type_class(GTK_TYPE_EDITABLE))->insert_text(editable,
                                                              new_text,
                                                              new_text_length,
                                                              position);
                                                                               
                                                                               
  }
  g_object_unref(editable);
  return TRUE;
}

static gboolean 
select_icon(GtkIconList *iconlist, 
            GtkIconListItem *icon,
            GdkEvent *event, gpointer data)
{
  GtkIconFileSel *filesel;
  GdkModifierType mods;
  const gchar *path = NULL;
  gchar *real_path = NULL;
  gchar *full_path = NULL;
  const gchar *file = NULL;
  GtkFileListItem *item;
  gboolean return_val = FALSE;

  item = (GtkFileListItem *)icon->link;

  filesel = GTK_ICON_FILESEL(data);

  if(item->type != GTK_FILE_LIST_FOLDER){
    gtk_entry_set_text(GTK_ENTRY(filesel->file_entry), icon->label);
    return TRUE;
  }else{
    gtk_entry_set_text(GTK_ENTRY(filesel->file_entry), "");
  }

  if(!event) return FALSE;

  if(event->type == GDK_BUTTON_PRESS || event->type == GDK_2BUTTON_PRESS)
    gdk_window_get_pointer(event->button.window, NULL, NULL, &mods);
  else
    return FALSE; 

  path = gtk_file_list_get_path(GTK_FILE_LIST(filesel->file_list));
  file = gtk_file_list_get_filename(GTK_FILE_LIST(filesel->file_list));
  file = icon->label;

  if(strlen(path) == 1)
    full_path = g_strconcat(G_DIR_SEPARATOR_S,file,G_DIR_SEPARATOR_S,NULL);
  else
    full_path = g_strconcat(path,G_DIR_SEPARATOR_S,file,G_DIR_SEPARATOR_S,NULL);
  real_path = get_real_path((const gchar *)full_path);

  if(filesel->selection) g_free(filesel->selection);
  filesel->selection = NULL;
  if(item->type != GTK_FILE_LIST_FOLDER){
    filesel->selection = g_strdup(real_path);
  }

  if((mods & GDK_BUTTON1_MASK) && event->type == GDK_2BUTTON_PRESS){
    gtk_label_set_text(GTK_LABEL(filesel->path_label), "Scanning...");
    if(filesel->show_tree){
/* FIXME
      return_val = gtk_dir_tree_open_dir(GTK_DIR_TREE(filesel->dir_tree), real_path);
*/
    } else
      return_val = gtk_file_list_open_dir(GTK_FILE_LIST(filesel->file_list), real_path);

    update_history_combo(filesel, real_path);

    gtk_label_set_text(GTK_LABEL(filesel->path_label), real_path);
  }

  g_free(full_path);
  g_free(real_path);
  return (!return_val);
}

static gboolean 
entry_set_file(GtkWidget *widget, GdkEventKey *key, gpointer data)
{
  GtkIconFileSel *filesel = GTK_ICON_FILESEL(data);

  if(key->keyval != GDK_Return && key->keyval != GDK_KP_Enter) return FALSE;

/*  real_set_file(widget, data);
*/
  gtk_signal_emit_by_name(GTK_OBJECT(filesel->ok_button), "clicked", filesel);

  return FALSE;
}

static void
real_set_file(GtkWidget *widget, gpointer data)
{
  GtkIconFileSel *filesel;
  GtkIconListItem *item;
  GList *list;
  const gchar *c;
  gchar *last, *text;
  gchar *folder;
  gchar *file;
  gint nlen, file_len;

  filesel = (GtkIconFileSel *)data;

  c = gtk_entry_get_text(GTK_ENTRY(filesel->file_entry));
  folder = NULL;
  file = NULL;
  last = NULL;
  file_len = nlen = 0;

  while(*c != '\0' && *c != '\n' && c != NULL){
   nlen++;
   file_len++;
   folder = (char *)g_realloc(folder, (nlen+1)*sizeof(char));
   folder[nlen-1] = *c;
   folder[nlen]='\0';
   file = (char *)g_realloc(file, (file_len+1)*sizeof(char));
   file[file_len-1] = *c;
   file[file_len]='\0';
   if(*c == G_DIR_SEPARATOR){
       g_free(file);
       g_free(last);
       last = g_strdup(folder);
       file_len = 0;
       file = NULL;
   }
   c++;
  }

  if(last) gtk_icon_file_selection_open_dir(filesel, last); 

  if(file){
    list = GTK_ICON_LIST(filesel->file_list)->icons;
    while(list){
      item = (GtkIconListItem *)list->data;
      text = ((GtkFileListItem *)item->link)->file_name;
      if(strcmp(text, file) == 0){
         gtk_icon_list_select_icon(GTK_ICON_LIST(filesel->file_list), item);
         break;
      }
      list = list->next;
    }
  }


  g_free(folder);
  g_free(file);
  g_free(last);
}

static gboolean 
set_filter(GtkWidget *widget, GdkEventKey *key, gpointer data)
{
  GtkIconFileSel *filesel;

  if(key->keyval != GDK_Return && key->keyval != GDK_KP_Enter) return FALSE;

  filesel = (GtkIconFileSel *)data;
  gtk_file_list_set_filter(GTK_FILE_LIST(filesel->file_list), 
                           gtk_entry_get_text(GTK_ENTRY(widget)));

  return TRUE;
}

static void
open_dir(GtkWidget *widget, GtkCTreeNode *node, gint n, gpointer data)
{
  DIR *dir;
  GtkDirTreeNode *dirnode;
  gchar *path;
  const gchar *last_path;
  GtkIconFileSel *filesel;

  filesel = GTK_ICON_FILESEL(data);


  dirnode=gtk_ctree_node_get_row_data(GTK_CTREE(widget),node);

  path = dirnode->path;

  last_path = gtk_file_list_get_path(GTK_FILE_LIST(filesel->file_list));

  if(strcmp(last_path, G_DIR_SEPARATOR_S) !=0 && strcmp(last_path, path) == 0) return; 

  gtk_widget_unmap(filesel->file_list);

  if((dir = opendir(path)) == NULL){
    return;
  }
  closedir(dir);

  gtk_label_set_text(GTK_LABEL(filesel->path_label), "Scanning...");

  gtk_file_list_open_dir(GTK_FILE_LIST(filesel->file_list), path);

  update_history_combo(filesel, path);

  gtk_widget_map(filesel->file_list);

  gtk_label_set_text(GTK_LABEL(filesel->path_label), path);
}

gint
gtk_icon_file_selection_open_dir(GtkIconFileSel *filesel, const gchar *path)
{
  DIR *dir;
  gint return_val = TRUE;
  gchar *real_path = NULL;
 
  if(!path) return FALSE;
  real_path = get_real_path(path);

  if((dir = opendir(real_path)) == NULL){
    g_warning("Can not open folder: %s",real_path);
    g_free(real_path);
    return FALSE;
  }

  gtk_label_set_text(GTK_LABEL(filesel->path_label), "Scanning...");

  if(filesel->show_tree){
/* FIXME
    return_val = gtk_dir_tree_open_dir(GTK_DIR_TREE(filesel->dir_tree), real_path);
*/
  } else
    return_val = gtk_file_list_open_dir(GTK_FILE_LIST(filesel->file_list), real_path);

  gtk_label_set_text(GTK_LABEL(filesel->path_label), real_path);

  update_history_combo(filesel, (const gchar *)real_path);

  g_free(real_path);

  return return_val;
}

void
gtk_icon_file_selection_show_hidden(GtkIconFileSel *filesel, gboolean visible)
{
/* FIXME
    GTK_DIR_TREE(filesel->dir_tree)->show_hidden = visible;
*/
    GTK_FILE_LIST(filesel->file_list)->show_hidden = visible;
}

void
gtk_icon_file_selection_set_filter(GtkIconFileSel *filesel, const gchar *filter)
{
  GTK_FILE_LIST(filesel->file_list)->filter = g_strdup(filter);
  gtk_file_list_open_dir(GTK_FILE_LIST(filesel->file_list), GTK_FILE_LIST(filesel->file_list)->path);
  update_history_combo(filesel, GTK_FILE_LIST(filesel->file_list)->path);
  if(filter != NULL)
    gtk_entry_set_text(GTK_ENTRY(filesel->filter_entry),filter);
}

static gchar *
get_real_path(const gchar *full_path)
{
  gchar root[5], root1[5], root2[5], root3[5], root4[5];
  gchar *aux_path;
  gint length;

  /* GET ABSOLUTE PATH */

  sprintf(root,"%s",G_DIR_SEPARATOR_S);
  sprintf(root1,"%s.",G_DIR_SEPARATOR_S);
  sprintf(root2,"%s..",G_DIR_SEPARATOR_S);
  sprintf(root3,"%s..%s",G_DIR_SEPARATOR_S,G_DIR_SEPARATOR_S);
  sprintf(root4,"%s.%s",G_DIR_SEPARATOR_S,G_DIR_SEPARATOR_S);

  aux_path = g_strdup(full_path);
  length = strlen(aux_path);

  if(strcmp(aux_path + length - 2, root1) == 0){
     if(length == 2) {
        g_free(aux_path);
        aux_path = g_strdup(root);
     } else {
        aux_path[length - 2] = '\0';
     }
  } else if(strcmp(aux_path + length - 3, root2) == 0){
     if(length == 3) {
        g_free(aux_path);
        aux_path = g_strdup(root);
     } else {
        gint i = length - 4;
        while(i >= 0){
           if(aux_path[i] == root[0]){
                aux_path[i] = '\0';
                break;
           }
           i--;
        }
     }
  } else if(strcmp(aux_path + length - 4, root3) == 0){
     if(length == 4) {
        g_free(aux_path);
        aux_path = g_strdup(root);
     } else {
        gint i = length - 5;
        while(i >= 0){
           if(aux_path[i] == root[0]){
                aux_path[i] = '\0';
                break;
           }
           i--;
        }
     }
  } else if(strcmp(aux_path + length - 3, root4) == 0){
     if(length == 3) {
        g_free(aux_path);
        aux_path = g_strdup(root);
     } else {
        aux_path[length - 3] = '\0';
     }
  } else if(strcmp(aux_path + length - 1, root) == 0 && length > 1){
     aux_path[length - 1] = '\0';
  }

  if(strlen(aux_path) == 0)
  {
    g_free(aux_path);
    aux_path = g_strdup(G_DIR_SEPARATOR_S);
  }

  return(aux_path);
}

static void
init_history_combo(GtkIconFileSel *filesel, const gchar *current_directory)
{
  GtkCombo *combo;
  GtkList *list;
  gchar *current_dir;
  gint dir_len;
  gint i;

  combo = GTK_COMBO(filesel->history_combo);
  list = GTK_LIST(combo->list);

  current_dir = g_strdup (current_directory);
  dir_len = strlen (current_dir);

  for (i = dir_len - 1; i >= 0; i--){

    /* the i == dir_len is to catch the full path for the first
     * entry. */
    if ( current_dir[i] == G_DIR_SEPARATOR )
    {
          GtkWidget *item;

          current_dir[i + 1] = '\0';
          item = gtk_list_item_new_with_label(current_dir);
          gtk_widget_show(item);
          gtk_container_add(GTK_CONTAINER(list), item);
     }
  }

  g_free(current_dir);
}


static void
update_history_combo(GtkIconFileSel *filesel, const gchar *current_directory)
{
  GtkCombo *combo;
  GtkList *list;
  GList *children;
  GtkWidget *item;

  combo = GTK_COMBO(filesel->history_combo);
  list = GTK_LIST(combo->list);

  gtk_entry_set_text(GTK_ENTRY(combo->entry), current_directory);
  children = list->children;
  while(children){
    GtkWidget *label;
    gchar *path;

    label = GTK_BIN (children->data)->child;
    if (label && GTK_IS_LABEL (label)){
      gtk_label_get (GTK_LABEL (label), &path);
      if(strcmp(path, current_directory) == 0) return;
    }

    children = children->next;
  }

  item = gtk_list_item_new_with_label(current_directory);
  gtk_widget_show(item);
  gtk_container_add(GTK_CONTAINER(list), item);
}

static void
go_to_history(GtkEntry *entry, gpointer data)
{
  const gchar *path;
  gchar *real_path;

  path = gtk_entry_get_text(entry);
  if(path[strlen(path)-1] != G_DIR_SEPARATOR)
    real_path = g_strconcat(path, G_DIR_SEPARATOR_S, NULL);
  else
    real_path = g_strdup(path);
  gtk_icon_file_selection_open_dir(GTK_ICON_FILESEL(data), real_path);
  g_free(real_path);
}

static gboolean 
combo_changed(GtkWidget *widget, GtkWidget *child, gpointer data)
{
  GtkCombo *combo;
  GtkEntry *entry;
  GtkIconFileSel *filesel;

  filesel = GTK_ICON_FILESEL(data);
  combo = GTK_COMBO(filesel->history_combo);
  entry = GTK_ENTRY(combo->entry);

  gtk_signal_handler_block (GTK_OBJECT (combo->list), combo->list_change_id);
  go_to_history(entry, filesel);
  gtk_signal_handler_unblock (GTK_OBJECT (combo->list), combo->list_change_id);

  return TRUE;
}


static gboolean 
entry_key_press(GtkWidget *widget, 
	        GdkEventKey *event, 
		gpointer data)
{
  GtkEntry *entry;
  entry = GTK_ENTRY (widget);
  if(event->keyval == GDK_Return){
     gtk_signal_emit_stop_by_name(GTK_OBJECT(entry), "key_press_event");
     go_to_history(entry, data);
     return TRUE;
  }
  return FALSE;
} 

const gchar *
gtk_icon_file_selection_get_selection(GtkIconFileSel *filesel)
{
  return filesel->selection;
}
