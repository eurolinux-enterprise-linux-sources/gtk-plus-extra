#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>
#include "gtkcombobutton.h"
#include "gtkbordercombo.h"
#include "gtkcolorcombo.h"
#include "gtktogglecombo.h"
#include "gtksheet.h"
#include "gtkitementry.h"
#include "gtkfontcombo.h"
#include "pixmaps.h"

#define DEFAULT_PRECISION 3
#define DEFAULT_SPACE 8
#define NUM_SHEETS 3 

GtkWidget *window;
GtkWidget *main_vbox;
GtkWidget *notebook;
GtkWidget **sheets;
GtkWidget **scrolled_windows;
GtkWidget *show_hide_box;
GtkWidget *status_box;
GtkWidget *location;
GtkWidget *entry;
GtkWidget *fgcolorcombo;
GtkWidget *bgcolorcombo;
GtkWidget *bordercombo;
GdkPixmap *pixmap;
GdkBitmap *mask;
GtkWidget *bg_pixmap;
GtkWidget *fg_pixmap;
GtkWidget *toolbar;
GtkWidget *left_button;
GtkWidget *center_button;
GtkWidget *right_button;
GtkWidget *tpixmap;
GtkWidget *bullet[10];
GtkWidget *smile;
GtkWidget *curve;
GtkWidget *popup;


void
quit ()
{
  gtk_main_quit();
}

static gint
popup_activated(GtkWidget *widget, gpointer data)
{
 GtkSheet *sheet;
 gint cur_page;
 gchar *item;

 cur_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
 sheet=GTK_SHEET(sheets[cur_page]);

 item = (gchar *)data;

 if(strcmp(item,"Add Column")==0)
   gtk_sheet_add_column(sheet,1);

 if(strcmp(item,"Add Row")==0)
   gtk_sheet_add_row(sheet,1);

 if(strcmp(item,"Insert Row")==0){
   if(sheet->state==GTK_SHEET_ROW_SELECTED)
     gtk_sheet_insert_rows(sheet,sheet->range.row0,                       
                               sheet->range.rowi-sheet->range.row0+1);
 }

 if(strcmp(item,"Insert Column")==0){
   if(sheet->state==GTK_SHEET_COLUMN_SELECTED)
     gtk_sheet_insert_columns(sheet,sheet->range.col0,                       
                              sheet->range.coli-sheet->range.col0+1);

 } 

 if(strcmp(item,"Delete Row")==0){
   if(sheet->state==GTK_SHEET_ROW_SELECTED)
     gtk_sheet_delete_rows(sheet,sheet->range.row0,
                              sheet->range.rowi-sheet->range.row0+1);
 }

 if(strcmp(item,"Delete Column")==0){
   if(sheet->state==GTK_SHEET_COLUMN_SELECTED)
     gtk_sheet_delete_columns(sheet,sheet->range.col0,
                              sheet->range.coli-sheet->range.col0+1);   
 } 

 if(strcmp(item,"Clear Cells")==0){
   if(sheet->state!=GTK_SHEET_NORMAL)
     gtk_sheet_range_clear(sheet, &sheet->range);
 } 

 gtk_widget_destroy(popup);
 return TRUE;
}

static GtkWidget *
build_menu(GtkWidget *sheet)
{
	static char *items[]={
		"Add Column",
		"Add Row",
		"Insert Row",
		"Insert Column",
		"Delete Row",
                "Delete Column",
                "Clear Cells"
	};
	GtkWidget *menu;
	GtkWidget *item;
	int i;

	menu=gtk_menu_new();

	for (i=0; i < (sizeof(items)/sizeof(items[0])) ; i++){
	 	item=gtk_menu_item_new_with_label(items[i]);

	 	gtk_signal_connect(GTK_OBJECT(item),"activate",
				   (GtkSignalFunc) popup_activated,
				   items[i]);

                GTK_WIDGET_SET_FLAGS (item, GTK_SENSITIVE | GTK_CAN_FOCUS);
                switch(i){
                  case 2:
                    if(GTK_SHEET(sheet)->state!=GTK_SHEET_ROW_SELECTED)
                     GTK_WIDGET_UNSET_FLAGS (item, 
                                            GTK_SENSITIVE | GTK_CAN_FOCUS);
                    break;
                  case 3:
                    if(GTK_SHEET(sheet)->state!=GTK_SHEET_COLUMN_SELECTED)
                     GTK_WIDGET_UNSET_FLAGS (item, 
                                            GTK_SENSITIVE | GTK_CAN_FOCUS);
                    break;
                  case 4:
                    if(GTK_SHEET(sheet)->state!=GTK_SHEET_ROW_SELECTED)
                     GTK_WIDGET_UNSET_FLAGS (item, 
                                            GTK_SENSITIVE | GTK_CAN_FOCUS);
                    break;
                  case 5:
                    if(GTK_SHEET(sheet)->state!=GTK_SHEET_COLUMN_SELECTED)
                     GTK_WIDGET_UNSET_FLAGS (item, 
                                            GTK_SENSITIVE | GTK_CAN_FOCUS);
                    break;
                } 

		gtk_widget_show(item);
	    	gtk_menu_append(GTK_MENU(menu),item);
	}

	return menu;
}

gint
do_popup(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
   GdkModifierType mods;
   GtkWidget *sheet;

   sheet=GTK_WIDGET(widget);

   gdk_window_get_pointer (sheet->window, NULL, NULL, &mods);
   if(mods&GDK_BUTTON3_MASK){ 

    if(popup)
       g_free(popup);

    popup=build_menu(sheet);

    gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL,
		   event->button, event->time);
   }

   return FALSE;
}

void
format_text (GtkSheet *sheet, const gchar *text, gint *justification, char *label)
{
  double auxval;
  int digspace=0;
  int cell_width, char_width;
  double val = 0.0;
  int format;
  double space;
  int intspace;
  int nonzero=FALSE;
  int ipos;
  char nchar;
  PangoContext *context;
  PangoFontMetrics *metrics;
  enum {EMPTY, TEXT, NUMERIC};

  context = gtk_widget_get_pango_context(GTK_WIDGET(sheet));
  metrics = pango_context_get_metrics(context, GTK_WIDGET(sheet)->style->font_desc, pango_context_get_language(context));
  char_width = pango_font_metrics_get_approximate_char_width(metrics);
  pango_font_metrics_unref(metrics);

  cell_width=sheet->column[sheet->active_cell.col].width;
  space= (double)cell_width/(double)char_width;

  intspace=MIN(space, DEFAULT_SPACE);

  format=EMPTY;
  if(strlen(text) != 0)
  { 
        for(ipos=0; ipos<strlen(text); ipos++){

          switch(nchar=text[ipos]){
           case '.':
           case ' ': case ',':
           case '-': case '+':
           case 'd': case 'D':
           case 'E': case 'e':
           case '1': case '2': case '3': case '4': case '5': case '6':
           case '7': case '8': case '9':
            nonzero=TRUE;
            break;
           case '0':
            break;
           default:
            format=TEXT;
          }
          if(format != EMPTY) break;
        }
        val=atof(text);
        if(format!=EMPTY || (val==0. && nonzero))
             format = TEXT;
        else
             format = NUMERIC;
  }

  switch(format){
    case TEXT:
    case EMPTY:
      strcpy(label, text);
      return;
    case NUMERIC:
      val=atof(text);
      *justification = GTK_JUSTIFY_RIGHT;
  }

  auxval= val < 0 ? -val : val;

  while(auxval<1 && auxval != 0.){
   auxval=auxval*10.;
   digspace+=1;
  }

  if(digspace+DEFAULT_PRECISION+1>intspace || digspace > DEFAULT_PRECISION){
    sprintf (label, "%*.*E", intspace, DEFAULT_PRECISION, val);
  }
  else
    {
    intspace=MIN(intspace, strlen(text)-digspace-1);
    sprintf (label, "%*.*f", intspace, DEFAULT_PRECISION, val);
    if(strlen(label) > space)
      sprintf (label, "%*.*E", intspace, DEFAULT_PRECISION, val);
  }
}

void
parse_numbers(GtkWidget *widget, gpointer data)
{
 GtkSheet *sheet;
 char label[400];
 gint justification;
 GtkSheetCellAttr attr;

 sheet=GTK_SHEET(widget);

 gtk_sheet_get_attributes(sheet, sheet->active_cell.row,
			         sheet->active_cell.col,
                                 &attr); 

 justification = attr.justification; 
 format_text(sheet, gtk_entry_get_text(GTK_ENTRY(sheet->sheet_entry)), 
             &justification, label);

 gtk_sheet_set_cell(sheet, sheet->active_cell.row,
			   sheet->active_cell.col,
                           justification, label); 

}

gint 
clipboard_handler(GtkWidget *widget, GdkEventKey *key)
{
  GtkSheet *sheet;

  sheet = GTK_SHEET(widget);

  if(key->state & GDK_CONTROL_MASK || key->keyval==GDK_Control_L ||
     key->keyval==GDK_Control_R){
    if((key->keyval=='c' || key->keyval == 'C') && sheet->state != GTK_STATE_NORMAL){
            if(gtk_sheet_in_clip(sheet)) gtk_sheet_unclip_range(sheet);
            gtk_sheet_clip_range(sheet, &sheet->range);
/*            gtk_sheet_unselect_range(sheet);
*/
    }
    if(key->keyval=='x' || key->keyval == 'X')
            gtk_sheet_unclip_range(sheet);    
  }

  return FALSE;
}

void 
resize_handler(GtkWidget *widget, GtkSheetRange *old_range, 
                                  GtkSheetRange *new_range, 
                                  gpointer data)
{
  printf("OLD SELECTION: %d %d %d %d\n",old_range->row0, old_range->col0,
                                    old_range->rowi, old_range->coli);
  printf("NEW SELECTION: %d %d %d %d\n",new_range->row0, new_range->col0,
                                    new_range->rowi, new_range->coli);


}

void 
move_handler(GtkWidget *widget, GtkSheetRange *old_range, 
                                  GtkSheetRange *new_range, 
                                  gpointer data)
{
  printf("OLD SELECTION: %d %d %d %d\n",old_range->row0, old_range->col0,
                                    old_range->rowi, old_range->coli);
  printf("NEW SELECTION: %d %d %d %d\n",new_range->row0, new_range->col0,
                                    new_range->rowi, new_range->coli);


}

gboolean
change_entry(GtkWidget *widget, 
             gint row, gint col, gint *new_row, gint *new_col,
             gpointer data)
{
  GtkSheet *sheet;

  sheet = GTK_SHEET(widget);

  if(*new_col == 0 && (col != 0 || sheet->state != GTK_STATE_NORMAL))
         gtk_sheet_change_entry(sheet, gtk_combo_get_type());

  if(*new_col == 1 && (col != 1 || sheet->state != GTK_STATE_NORMAL))
         gtk_sheet_change_entry(sheet, GTK_TYPE_ENTRY);

  if(*new_col == 2 && (col != 2 || sheet->state != GTK_STATE_NORMAL))
         gtk_sheet_change_entry(sheet, GTK_TYPE_SPIN_BUTTON);

  if(*new_col >= 3 && (col < 3 || sheet->state != GTK_STATE_NORMAL))
         gtk_sheet_change_entry(sheet, GTK_TYPE_ITEM_ENTRY);

  return TRUE;
}


void alarm_change(GtkWidget *widget, gint row, gint col, 
                  gpointer data)
{
 printf("CHANGE CELL: %d %d\n",row,col);
}

gboolean alarm_activate(GtkWidget *widget, gint row, gint col, 
                    gpointer data)
{
 GtkSheetRange range;

 printf("ACTIVATE CELL: %d %d\n",row,col);

 range.row0 = range.rowi = row;
 range.col0 = range.coli = col;
/*
 gtk_sheet_range_set_justification(GTK_SHEET(widget), &range, GTK_JUSTIFY_LEFT);
*/
 return TRUE;
}

gboolean alarm_deactivate(GtkWidget *widget, gint row, gint col, 
                      gpointer data)
{
 GtkSheetRange range;
 gchar *text;
 GtkSheet *sheet = GTK_SHEET(widget);

 printf("DEACTIVATE CELL: %d %d\n",row,col);

 range.row0 = range.rowi = row;
 range.col0 = range.coli = col;
/*
 gtk_sheet_range_set_justification(GTK_SHEET(widget), &range, GTK_JUSTIFY_RIGHT);
*/
  text = g_strdup(gtk_sheet_cell_get_text(GTK_SHEET(widget), row, col));

  if(text && strlen(text) > 0){
    gtk_sheet_set_cell_text(sheet, row, col, text);
    g_free(text);
  }

 return TRUE;
}

gboolean alarm_traverse(GtkWidget *widget, 
                    gint row, gint col, gint *new_row, gint *new_col,
                    gpointer data)
{
 printf("TRAVERSE: %d %d %d %d\n",row,col,*new_row,*new_col);
 return TRUE;
}

void show_child(GtkWidget *widget, gpointer data)
{
 if(!GTK_WIDGET_MAPPED(curve))
         gtk_sheet_attach_floating(GTK_SHEET(sheets[0]), curve, 2, 7);
/*         gtk_sheet_put(GTK_SHEET(sheets[0]), curve, 550, 120);
*/
}

void
build_example1(GtkWidget *widget)
{
 GtkSheet *sheet;
 GtkWidget *show_button;
 GtkSheetRange range;
 GdkRectangle area;
 GdkColor color;
 GdkColormap *colormap;
 PangoFontDescription *font;
 gchar font_name1[]="Arial 36";
 gchar font_name2[]="Arial 28";

 gchar name[10];
 gint i;

 sheet=GTK_SHEET(widget);
 colormap = gdk_colormap_get_system();

 gdk_color_parse("light yellow", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_set_background(sheet, &color);
 gdk_color_parse("light blue", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_set_grid(sheet, &color);

 for(i=0; i<=sheet->maxcol; i++){
   name[0]='A'+i;
   name[1]='\0';

   gtk_sheet_column_button_add_label(sheet, i, name);
   gtk_sheet_set_column_title(sheet, i, name);
 }

 gtk_sheet_row_button_add_label(sheet, 0, "This is\na multiline\nlabel");
 gtk_sheet_row_button_add_label(sheet, 1, "This is long label");
/*
 gtk_sheet_column_button_add_label(sheet, 7, "This is\na multiline\nlabel");
 gtk_sheet_column_button_add_label(sheet, 8, "This is long label");
*/ 
 gtk_sheet_row_button_justify(sheet, 0, GTK_JUSTIFY_RIGHT);

 range.row0=1;
 range.rowi=2;
 range.col0=1;
 range.coli=3;

 gtk_sheet_clip_range(sheet, &range);

 font = pango_font_description_from_string (font_name2);
 gtk_sheet_range_set_font(sheet, &range, font);

 gdk_color_parse("red", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_foreground(sheet, &range, &color);

 gtk_sheet_set_cell(sheet, 1,2, GTK_JUSTIFY_CENTER,
                    "Welcome to");

 range.row0=2;

 font = pango_font_description_from_string (font_name1);
 gtk_sheet_range_set_font(sheet, &range, font);

 gdk_color_parse("blue", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_foreground(sheet, &range, &color);

 gtk_sheet_set_cell(sheet, 2,2, GTK_JUSTIFY_CENTER, "GtkSheet");

 range.row0=3;
 range.rowi=3;
 range.col0=0;
 range.coli=4;
 gdk_color_parse("dark gray", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_background(sheet, &range, &color);
 gdk_color_parse("green", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_foreground(sheet, &range, &color);

 gtk_sheet_set_cell(sheet,3,2,GTK_JUSTIFY_CENTER,
"a Matrix widget for Gtk+");

 gtk_sheet_set_cell(sheet,4,1,GTK_JUSTIFY_LEFT,
"GtkSheet is a matrix where you can allocate cells of text.");
 gtk_sheet_set_cell(sheet,5,1,GTK_JUSTIFY_LEFT,
"Cell contents can be edited interactively with an specially designed entry");
 gtk_sheet_set_cell(sheet,6,1,GTK_JUSTIFY_LEFT,
"You can change colors, borders, and many other attributes");
 gtk_sheet_set_cell(sheet, 7,1, GTK_JUSTIFY_LEFT,
"Drag & drop or resize the selection clicking the corner or border");
 gtk_sheet_set_cell(sheet, 8, 1, GTK_JUSTIFY_LEFT,
"Store the selection on the clipboard pressing Ctrl-C");
 gtk_sheet_set_cell(sheet, 9, 1, GTK_JUSTIFY_LEFT,
 "(The selection handler has not been implemented yet)");
 gtk_sheet_set_cell(sheet, 10, 1, GTK_JUSTIFY_LEFT,
 "You can add buttons, charts, pixmaps, and other widgets");

 gtk_signal_connect(GTK_OBJECT(sheet),
                    "key_press_event",
                    (GtkSignalFunc) clipboard_handler, 
                    NULL);

 gtk_signal_connect(GTK_OBJECT(sheet),
                    "resize_range",
                    (GtkSignalFunc) resize_handler, 
                    NULL);

 gtk_signal_connect(GTK_OBJECT(sheet),
                    "move_range",
                    (GtkSignalFunc) move_handler, 
                    NULL);

 gtk_signal_connect(GTK_OBJECT(sheet),
                    "changed",
                    (GtkSignalFunc) alarm_change, 
                    NULL);

 gtk_signal_connect(GTK_OBJECT(sheet),
                    "activate",
                    (GtkSignalFunc) alarm_activate, 
                    NULL);

 gtk_signal_connect(GTK_OBJECT(sheet),
                    "deactivate",
                    (GtkSignalFunc) alarm_deactivate, 
                    NULL);

 gtk_signal_connect(GTK_OBJECT(sheet),
                    "traverse",
                    (GtkSignalFunc) alarm_traverse, 
                    NULL);

 curve=gtk_curve_new();
 gtk_curve_set_range(GTK_CURVE(curve), 0, 200, 0, 200);
 gtk_widget_show(curve);

 pixmap=gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
 				   bullet_xpm);

 for(i=0; i<5; i++){
      bullet[i] = gtk_pixmap_new(pixmap, mask);
      gtk_widget_show(bullet[i]);
      gtk_sheet_get_cell_area(GTK_SHEET(sheets[0]), 4+i, 0, &area);
/*      gtk_sheet_put(GTK_SHEET(sheets[0]), bullet[i], 
                    area.x+area.width/2-8, area.y+area.height/2-8);
*/
      gtk_sheet_attach(GTK_SHEET(sheets[0]), bullet[i], 4+i, 0, GTK_EXPAND, GTK_EXPAND, 0, 0);
 }

 bullet[5] = gtk_pixmap_new(pixmap, mask);
 gtk_widget_show(bullet[5]);
 gtk_sheet_get_cell_area(GTK_SHEET(sheets[0]), 10, 0, &area);
/* gtk_sheet_put(GTK_SHEET(sheets[0]), bullet[i], 
               area.x+area.width/2-8, area.y+area.height/2-8);
*/
 gtk_sheet_attach(GTK_SHEET(sheets[0]), bullet[5], 10, 0, GTK_EXPAND, GTK_EXPAND, 0, 0);
 gdk_pixmap_unref(pixmap);
 gdk_bitmap_unref(mask);


 pixmap=gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
 				   smile_xpm);
 smile = gtk_pixmap_new(pixmap, mask);
 gtk_widget_show(smile);
 gtk_sheet_button_attach(GTK_SHEET(sheets[0]), smile, -1, 5);
 gdk_pixmap_unref(pixmap);
 gdk_bitmap_unref(mask);


 show_button=gtk_button_new_with_label("Show me a plot");
 gtk_widget_show(show_button);
 gtk_widget_set_usize(show_button, 100,60);
 gtk_sheet_get_cell_area(GTK_SHEET(sheets[0]), 12, 2, &area);
/* gtk_sheet_put(GTK_SHEET(sheets[0]), show_button, area.x, area.y);
*/
 gtk_sheet_attach(GTK_SHEET(sheets[0]), show_button, 12, 2, GTK_FILL, GTK_FILL, 5, 5);

 gtk_signal_connect(GTK_OBJECT(show_button), "clicked",
                    (GtkSignalFunc) show_child, 
                    NULL);
/*
 gtk_signal_connect(GTK_OBJECT(sheet),
                    "button_press_event",
                    (GtkSignalFunc) do_popup, 
                    NULL);
*/


}


void
build_example2(GtkWidget *widget)
{
 GtkSheet *sheet;
 GtkSheetRange range;
 GdkColor color;
 GtkWidget *b;
 
 sheet=GTK_SHEET(widget);
 gtk_sheet_set_autoscroll(sheet, TRUE);
/*
 gtk_sheet_set_justify_entry(sheet, TRUE);
*/
 gtk_sheet_set_selection_mode(sheet, GTK_SELECTION_SINGLE);

 range.row0=0;
 range.rowi=2;
 range.col0=0;
 range.coli=sheet->maxcol;
 gtk_sheet_range_set_editable(sheet, &range, FALSE);
 gdk_color_parse("light gray", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_background(sheet, &range, &color);
 gdk_color_parse("blue", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_foreground(sheet, &range, &color);
 range.row0=1;
 gdk_color_parse("red", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_foreground(sheet, &range, &color);
 range.row0=2;
 gdk_color_parse("black", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_foreground(sheet, &range, &color);

/*
 gtk_sheet_row_set_sensitivity(sheet, 0, FALSE);
 gtk_sheet_row_set_sensitivity(sheet, 1, FALSE);
 gtk_sheet_row_set_sensitivity(sheet, 2, FALSE);
*/
 gtk_sheet_set_cell(sheet, 0,2, GTK_JUSTIFY_CENTER,
                    "Click the right mouse button to display a popup");
 gtk_sheet_set_cell(sheet, 1,2, GTK_JUSTIFY_CENTER,
                    "You can connect a parser to the 'set cell' signal");
 gtk_sheet_set_cell(sheet, 2,2, GTK_JUSTIFY_CENTER,
                    "(Try typing numbers)");
 gtk_sheet_set_active_cell(sheet, 3, 0);

/*
 gtk_sheet_set_update_policy(sheet, GTK_UPDATE_CONTINUOUS, GTK_UPDATE_CONTINUOUS);
*/

 gtk_signal_connect(GTK_OBJECT(sheet),
                    "button_press_event",
                    (GtkSignalFunc) do_popup, 
                    NULL);

 gtk_signal_connect(GTK_OBJECT(sheet),
	            "set_cell",
		    (GtkSignalFunc) parse_numbers,
                    NULL);                   

 gtk_signal_connect(GTK_OBJECT(sheet),
                    "activate",
                    (GtkSignalFunc) alarm_activate, 
                    NULL);

 gtk_signal_connect(GTK_OBJECT(sheet),
                    "deactivate",
                    (GtkSignalFunc) alarm_deactivate, 
                    NULL);

 gtk_signal_connect(GTK_OBJECT(sheet),
                    "traverse",
                    (GtkSignalFunc) alarm_traverse, 
                    NULL);

 gtk_sheet_set_row_height(GTK_SHEET(sheets[1]), 12, 60);
 b = gtk_button_new_with_label("GTK_FILL");
 gtk_sheet_attach(GTK_SHEET(sheets[1]), b, 12, 2, GTK_FILL, GTK_FILL, 5, 5);
 gtk_widget_show(b);
 b = gtk_button_new_with_label("GTK_EXPAND");
 gtk_sheet_attach(GTK_SHEET(sheets[1]), b, 12, 3, GTK_EXPAND, GTK_EXPAND, 5, 5); gtk_widget_show(b);
 b = gtk_button_new_with_label("GTK_SHRINK");
 gtk_sheet_attach(GTK_SHEET(sheets[1]), b, 12, 4, GTK_SHRINK, GTK_SHRINK, 5, 5); gtk_widget_show(b);
}

void
build_example3(GtkWidget *widget)
{
 GtkSheet *sheet;
 GtkSheetRange range;
 GdkColor color;
 
 sheet=GTK_SHEET(widget);

 gtk_sheet_show_grid(sheet, FALSE);

 range.row0=0;
 range.rowi=10;
 range.col0=0;
 range.coli=6;
 gdk_color_parse("orange", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_background(sheet, &range, &color);
 gdk_color_parse("violet", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_foreground(sheet, &range, &color);
 range.row0=1;
 gdk_color_parse("blue", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_background(sheet, &range, &color);
 range.coli=0;
 gdk_color_parse("dark green", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_background(sheet, &range, &color);

 range.row0=0; 
 gdk_color_parse("dark blue", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_border_color(sheet, &range, &color);
 gtk_sheet_range_set_border(sheet, &range, GTK_SHEET_RIGHT_BORDER, 4, 1);
 range.coli=0;
 range.col0=0;
 range.rowi=0;
 gdk_color_parse("red", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_background(sheet, &range, &color);
 gtk_sheet_range_set_border(sheet, &range, GTK_SHEET_RIGHT_BORDER|
                                          GTK_SHEET_BOTTOM_BORDER, 4, 0);
 range.rowi=0;
 range.col0=1;
 range.coli=6;
 gdk_color_parse("dark blue", &color);
 gdk_color_alloc(gtk_widget_get_colormap(widget), &color);
 gtk_sheet_range_set_border_color(sheet, &range, &color);
 gtk_sheet_range_set_border(sheet, &range, GTK_SHEET_BOTTOM_BORDER, 4, 1);

 gtk_sheet_set_autoresize(sheet, TRUE);

 gtk_sheet_change_entry(sheet, gtk_combo_get_type());


 gtk_signal_connect(GTK_OBJECT(sheet),
                    "traverse",
                    (GtkSignalFunc) change_entry, 
                    NULL);

}

/*
void
build_example4(GtkWidget *widget)
{
 GtkSheet *sheet;

 sheet=GTK_SHEET(widget);
 gtk_signal_connect(GTK_OBJECT(sheet),
                    "button_press_event",
                    (GtkSignalFunc) do_popup, 
                    NULL);
}
*/

void
set_cell(GtkWidget *widget, gchar *insert, gint text_legth, gint position, 
         gpointer data)
{
  const char *text;
  GtkEntry *sheet_entry;

  sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(GTK_SHEET(widget)));

  if((text = gtk_entry_get_text (sheet_entry))) {
                          gtk_entry_set_text(GTK_ENTRY(entry), text);
  }
  GTK_WIDGET_UNSET_FLAGS(entry, GTK_HAS_FOCUS);
  GTK_WIDGET_SET_FLAGS(GTK_SHEET(widget)->sheet_entry, GTK_HAS_FOCUS);
} 

void
show_sheet_entry(GtkWidget *widget, gpointer data)
{
 const char *text;
 GtkSheet *sheet;
 GtkEntry *sheet_entry;
 gint cur_page;

 if(!GTK_WIDGET_HAS_FOCUS(widget)) return;

 cur_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
 sheet=GTK_SHEET(sheets[cur_page]);
 sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(sheet));

 if((text=gtk_entry_get_text (GTK_ENTRY(entry)))){
   gtk_entry_set_text(sheet_entry, text);
 }
}

void 
activate_sheet_entry(GtkWidget *widget, gpointer data)
{
  GtkSheet *sheet;
  GtkEntry *sheet_entry;
  gint cur_page, row, col;
  gint justification=GTK_JUSTIFY_LEFT;
  
  cur_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  sheet=GTK_SHEET(sheets[cur_page]);
  row=sheet->active_cell.row; col=sheet->active_cell.col;

  sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(sheet));

  if(GTK_IS_ITEM_ENTRY(sheet_entry))
         justification = GTK_ITEM_ENTRY(sheet_entry)->justification;

  gtk_sheet_set_cell(sheet, row, col,
		     justification,
		     gtk_entry_get_text (sheet_entry));

}

void 
show_entry(GtkWidget *widget, gpointer data)
{
 const char *text; 
 GtkSheet *sheet;
 GtkWidget * sheet_entry;
 gint cur_page;

 if(!GTK_WIDGET_HAS_FOCUS(widget)) return;

 cur_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
 sheet=GTK_SHEET(sheets[cur_page]);
 sheet_entry = gtk_sheet_get_entry(sheet);

 if((text=gtk_entry_get_text (GTK_ENTRY(sheet_entry))))
			  gtk_entry_set_text(GTK_ENTRY(entry), text);

}

void 
justify_left(GtkWidget *widget)
{
  GtkSheet *current;
  gint cur_page;

  cur_page=gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  current=GTK_SHEET(sheets[cur_page]);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(left_button),
  GTK_STATE_ACTIVE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(right_button),
  GTK_STATE_NORMAL);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(center_button),
  GTK_STATE_NORMAL);

  gtk_sheet_range_set_justification(current, &current->range,
  GTK_JUSTIFY_LEFT);
}

void 
justify_center(GtkWidget *widget) 
{
  GtkSheet *current;
  gint cur_page;

  cur_page=gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  current=GTK_SHEET(sheets[cur_page]);

  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(center_button),
  GTK_STATE_ACTIVE);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(right_button),
  GTK_STATE_NORMAL);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(left_button),
  GTK_STATE_NORMAL);

  gtk_sheet_range_set_justification(current, &current->range,
  GTK_JUSTIFY_CENTER);
}

void 
justify_right(GtkWidget *widget) 
{
  GtkSheet *current;
  gint cur_page;

  cur_page=gtk_notebook_current_page(GTK_NOTEBOOK(notebook));
  current=GTK_SHEET(sheets[cur_page]);

  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(right_button),
  GTK_STATE_ACTIVE);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(left_button),
  GTK_STATE_NORMAL);
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(center_button),
  GTK_STATE_NORMAL);

  gtk_sheet_range_set_justification(current, &current->range,
  GTK_JUSTIFY_RIGHT);
}

gint
activate_sheet_cell(GtkWidget *widget, gint row, gint column, gpointer data) 
{
  GtkSheet *sheet;
  GtkEntry *sheet_entry;
  char cell[100];
  const char *text;
  GtkSheetCellAttr attributes;

  sheet=GTK_SHEET(widget);
  sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(sheet));

  if(GTK_SHEET(widget)->column[column].name)
   sprintf(cell,"  %s:%d  ",GTK_SHEET(widget)->column[column].name, row);
  else
   sprintf(cell, " ROW: %d COLUMN: %d ", row, column);

  gtk_label_set(GTK_LABEL(location), cell);

  gtk_entry_set_max_length(GTK_ENTRY(entry),
	GTK_ENTRY(sheet_entry)->text_max_length);

  if((text=gtk_entry_get_text(GTK_ENTRY(gtk_sheet_get_entry(sheet)))))
			  gtk_entry_set_text(GTK_ENTRY(entry), text);
  else
			  gtk_entry_set_text(GTK_ENTRY(entry), "");

  gtk_sheet_get_attributes(sheet,sheet->active_cell.row,
				sheet->active_cell.col, &attributes);

  gtk_entry_set_editable(GTK_ENTRY(entry), attributes.is_editable);

  switch (attributes.justification){
    case GTK_JUSTIFY_LEFT:
         justify_left(NULL);
	 break;
    case GTK_JUSTIFY_CENTER:
         justify_center(NULL);
	 break;
    case GTK_JUSTIFY_RIGHT:
         justify_right(NULL);
	 break;
    default:
         justify_left(NULL);
	 break;
  }

  return TRUE;
}

void 
change_border (GtkWidget *widget, gint border)
{
   GtkSheet *current;
   gint cur_page;
   GtkSheetRange range, auxrange;
   gint border_mask, border_width=3;
   gint auxcol, auxrow;
   gint i,j;

   cur_page=gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
   current=GTK_SHEET(sheets[cur_page]);

   range=current->range;

   gtk_sheet_range_set_border(current, &range, 0, 0, 0);

   switch(border){
     case 0:
       break;
     case 1:
       border_mask = GTK_SHEET_TOP_BORDER;
       range.rowi = range.row0;
       gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
       break;
     case 2:
       border_mask = GTK_SHEET_BOTTOM_BORDER;
       range.row0 = range.rowi;
       gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
       break;
     case 3:
       border_mask = GTK_SHEET_RIGHT_BORDER;
       range.col0 = range.coli;
       gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
       break;
     case 4:
       border_mask = GTK_SHEET_LEFT_BORDER;
       range.coli = range.col0;
       gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
       break;
     case 5:
       if(range.col0 == range.coli){
	   border_mask = GTK_SHEET_LEFT_BORDER | GTK_SHEET_RIGHT_BORDER;
	   gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
           break;
	}
       border_mask = GTK_SHEET_LEFT_BORDER;
       auxcol=range.coli;
       range.coli = range.col0;
       gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
       border_mask = GTK_SHEET_RIGHT_BORDER;
       range.col0 = range.coli=auxcol; 
       gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
       break;
     case 6:
       if(range.row0 == range.rowi){
	   border_mask = GTK_SHEET_TOP_BORDER | GTK_SHEET_BOTTOM_BORDER;
	   gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
           break;
	}
       border_mask = GTK_SHEET_TOP_BORDER;
       auxrow=range.rowi;
       range.rowi = range.row0;
       gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
       border_mask = GTK_SHEET_BOTTOM_BORDER;
       range.row0=range.rowi=auxrow; 
       gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
       break;
     case 7:
       border_mask = GTK_SHEET_RIGHT_BORDER | GTK_SHEET_LEFT_BORDER;
       gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
       break;
     case 8:
       border_mask = GTK_SHEET_BOTTOM_BORDER | GTK_SHEET_TOP_BORDER;
       gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
       break;
     case 9:
       gtk_sheet_range_set_border(current, &range, 15, border_width, 0);
       for(i=range.row0; i<=range.rowi; i++)
        for(j=range.col0; j<=range.coli; j++){
          border_mask = 15;
          auxrange.row0 = i;
          auxrange.rowi = i;
          auxrange.col0 = j;
          auxrange.coli = j;
          if(i == range.rowi) border_mask ^= GTK_SHEET_BOTTOM_BORDER;
          if(i == range.row0) border_mask ^= GTK_SHEET_TOP_BORDER;
          if(j == range.coli) border_mask ^= GTK_SHEET_RIGHT_BORDER;
          if(j == range.col0) border_mask ^= GTK_SHEET_LEFT_BORDER;
          if(border_mask != 15)
            gtk_sheet_range_set_border(current, &auxrange, border_mask, 
                                       border_width, 0);
       }

       break;
     case 10:
       for(i=range.row0; i<=range.rowi; i++)
        for(j=range.col0; j<=range.coli; j++){
          border_mask = 0;
          auxrange.row0 = i;
          auxrange.rowi = i;
          auxrange.col0 = j;
          auxrange.coli = j;
          if(i == range.rowi) border_mask |= GTK_SHEET_BOTTOM_BORDER;
          if(i == range.row0) border_mask |= GTK_SHEET_TOP_BORDER;
          if(j == range.coli) border_mask |= GTK_SHEET_RIGHT_BORDER;
          if(j == range.col0) border_mask |= GTK_SHEET_LEFT_BORDER;
          if(border_mask != 0)
            gtk_sheet_range_set_border(current, &auxrange, border_mask, 
                                       border_width, 0);
       }

       break;
     case 11:
       border_mask = 15;
       gtk_sheet_range_set_border(current, &range, border_mask, border_width, 0);
       break;


   }

}


void 
change_fg(GtkWidget *widget, gint i, GdkColor *color)
{
   GtkSheet *current;
   gint cur_page;
   GdkGC *tmp_gc;

   cur_page=gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
   current=GTK_SHEET(sheets[cur_page]);

   gtk_sheet_range_set_foreground(current, &current->range, color);

   tmp_gc=gdk_gc_new(window->window);
   gdk_gc_set_foreground(tmp_gc,color);
   gdk_draw_rectangle(GTK_PIXMAP(fg_pixmap)->pixmap,
		      tmp_gc, TRUE, 5,20,16,4);
   gtk_widget_queue_draw(fg_pixmap); 
   gdk_gc_unref(tmp_gc);
}

void 
change_bg(GtkWidget *widget, gint i, GdkColor *color)
{
   GtkSheet *current;
   gint cur_page;
   GdkGC *tmp_gc;

   cur_page=gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
   current=GTK_SHEET(sheets[cur_page]);

   gtk_sheet_range_set_background(current, &current->range, color);

   tmp_gc=gdk_gc_new(window->window);
   gdk_gc_set_foreground(tmp_gc,color);
   gdk_draw_rectangle(GTK_PIXMAP(bg_pixmap)->pixmap,
		      tmp_gc, TRUE, 4,20,18,4);
   gtk_widget_draw(bg_pixmap, NULL); 
   gdk_gc_unref(tmp_gc);
}

void 
do_hide_row_titles(GtkWidget *widget) 
{
  GtkSheet *current;
  gint cur_page;

  cur_page=gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  current=GTK_SHEET(sheets[cur_page]);

  gtk_sheet_hide_row_titles(current);
}

void 
do_hide_column_titles(GtkWidget *widget) 
{
  GtkSheet *current; 
  gint cur_page;

  cur_page=gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  current=GTK_SHEET(sheets[cur_page]);

  gtk_sheet_hide_column_titles(current);

}

void 
do_show_row_titles(GtkWidget *widget) 
{
  GtkSheet *current; 
  gint cur_page;

  cur_page=gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  current=GTK_SHEET(sheets[cur_page]);

  gtk_sheet_show_row_titles(current);
}

void 
do_show_column_titles(GtkWidget *widget) 
{
  GtkSheet *current; 
  gint cur_page;

  cur_page=gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  current=GTK_SHEET(sheets[cur_page]);

  gtk_sheet_show_column_titles(current);

}

void
new_font(GtkFontCombo *font_combo, gpointer data)
{
  GtkSheet *current; 
  gint cur_page;

  cur_page=gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
  current=GTK_SHEET(sheets[cur_page]);

  gtk_sheet_range_set_font(current, &current->range, gtk_font_combo_get_font_description(font_combo));
}

int main(int argc, char *argv[]) 
{
  GtkWidget *label; 
  GtkRequisition request; 
  GtkWidget *hide_row_titles;
  GtkWidget *hide_column_titles; 
  GtkWidget *show_row_titles; 
  GtkWidget *show_column_titles;
  GtkWidget *font_combo;
  GtkWidget *toggle_combo;
  GdkColormap *colormap;
  gint i;

  char *title[]= {"Example 1",
		  "Example 2", 
		  "Example 3", 
                  "Example 4"};
  char *folder[]= {"Folder 1",
		   "Folder 2", 
		   "Folder 3", 
                   "Folder 4"};

  gtk_init(&argc, &argv);

  colormap = gdk_colormap_get_system();

  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "GtkSheet Demo");
  gtk_widget_set_usize(GTK_WIDGET(window), 900, 600);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (quit), NULL);

  main_vbox=gtk_vbox_new(FALSE,1);
  gtk_container_set_border_width(GTK_CONTAINER(main_vbox),0); 
  gtk_container_add(GTK_CONTAINER(window), main_vbox);
  gtk_widget_show(main_vbox);

  show_hide_box = gtk_hbox_new(FALSE, 1); 
  hide_row_titles = gtk_button_new_with_label("Hide Row Titles"); 
  hide_column_titles = gtk_button_new_with_label("Hide Column Titles"); 
  show_row_titles = gtk_button_new_with_label("Show Row Titles"); 
  show_column_titles = gtk_button_new_with_label("Show Column Titles");

  gtk_box_pack_start(GTK_BOX(show_hide_box), hide_row_titles, TRUE, TRUE, 0); 
  gtk_box_pack_start(GTK_BOX(show_hide_box), hide_column_titles, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(show_hide_box), show_row_titles, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(show_hide_box), show_column_titles, TRUE, TRUE, 0);

  gtk_widget_show(hide_row_titles); 
  gtk_widget_show(hide_column_titles);
  gtk_widget_show(show_row_titles); 
  gtk_widget_show(show_column_titles);

  gtk_signal_connect(GTK_OBJECT(hide_row_titles), "clicked",
		     (GtkSignalFunc) do_hide_row_titles, NULL);

  gtk_signal_connect(GTK_OBJECT(hide_column_titles), "clicked",
		     (GtkSignalFunc) do_hide_column_titles, NULL);

  gtk_signal_connect(GTK_OBJECT(show_row_titles), "clicked",
		     (GtkSignalFunc) do_show_row_titles, NULL);

  gtk_signal_connect(GTK_OBJECT(show_column_titles), "clicked",
		     (GtkSignalFunc) do_show_column_titles, NULL);

  gtk_box_pack_start(GTK_BOX(main_vbox), show_hide_box, FALSE, TRUE, 0);
  gtk_widget_show(show_hide_box);

  toolbar=gtk_toolbar_new();

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  font_combo = gtk_font_combo_new();
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar),
			    font_combo, "font", "font");

  gtk_widget_set_usize(GTK_FONT_COMBO(font_combo)->italic_button, 32, 32);
  gtk_widget_set_usize(GTK_FONT_COMBO(font_combo)->bold_button, 32, 32);
  gtk_widget_show(font_combo);
  gtk_signal_connect(GTK_OBJECT(font_combo), "changed", GTK_SIGNAL_FUNC(new_font), NULL);

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  left_button = gtk_toggle_button_new();
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar),
			    left_button, "justify left", "justify left");
  gtk_widget_show(left_button);

  gtk_signal_connect(GTK_OBJECT(left_button),"released",
		     (GtkSignalFunc) justify_left, NULL);

  center_button = gtk_toggle_button_new();
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar),
			    center_button, "justify center", "justify center");
  gtk_widget_show(center_button);

  gtk_signal_connect(GTK_OBJECT(center_button), "released",
		     (GtkSignalFunc) justify_center, NULL);


  right_button = gtk_toggle_button_new();
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar),
			    right_button, "justify right", "justify right");
  gtk_widget_show(right_button);

  gtk_signal_connect(GTK_OBJECT(right_button), "released",
		     (GtkSignalFunc) justify_right, NULL);

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  bordercombo=gtk_border_combo_new();
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar),
			    bordercombo, "border", "border");
  gtk_widget_set_usize(GTK_COMBO_BUTTON(bordercombo)->button, 32, 32);
  gtk_widget_show(bordercombo);

  gtk_signal_connect(GTK_OBJECT(bordercombo),
		     "changed", (GtkSignalFunc)change_border, NULL);

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  fgcolorcombo=gtk_color_combo_new();
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar),
			    fgcolorcombo, "font color", "font color");
  gtk_widget_show(fgcolorcombo);

  gtk_signal_connect(GTK_OBJECT(fgcolorcombo),
		     "changed", (GtkSignalFunc)change_fg, NULL);

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  bgcolorcombo=gtk_color_combo_new();
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar),
			    bgcolorcombo, "background color", "background color");
  gtk_widget_show(bgcolorcombo);

  gtk_signal_connect(GTK_OBJECT(bgcolorcombo),
		     "changed", (GtkSignalFunc)change_bg, NULL);

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  toggle_combo = gtk_toggle_combo_new(5, 5);
  gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar),
			    toggle_combo, "test", "test");
  gtk_widget_set_usize(GTK_COMBO_BUTTON(toggle_combo)->button, 32, 32);
  gtk_widget_show(toggle_combo);

  gtk_box_pack_start(GTK_BOX(main_vbox), toolbar, FALSE, TRUE, 0);
  gtk_widget_show(toolbar);

  status_box=gtk_hbox_new(FALSE, 1);
  gtk_container_set_border_width(GTK_CONTAINER(status_box),0);
  gtk_box_pack_start(GTK_BOX(main_vbox), status_box, FALSE, TRUE, 0);
  gtk_widget_show(status_box);

  location=gtk_label_new(""); 
  gtk_widget_size_request(location, &request); 
  gtk_widget_set_usize(location, 160, request.height);
  gtk_box_pack_start(GTK_BOX(status_box), location, FALSE, TRUE, 0);
  gtk_widget_show(location);

  entry=gtk_entry_new(); 
  gtk_box_pack_start(GTK_BOX(status_box), entry, TRUE, TRUE, 0); 
  gtk_widget_show(entry);

  notebook=gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_BOTTOM);
  gtk_box_pack_start(GTK_BOX(main_vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show(notebook);

  for(i=0; i<NUM_SHEETS; i++){
   sheets=(GtkWidget **)realloc(sheets, (i+1)*sizeof(GtkWidget *));

   sheets[i]=gtk_sheet_new(1000,26,title[i]);

   scrolled_windows=(GtkWidget **)realloc(scrolled_windows, (i+1)*sizeof(GtkWidget *));
   scrolled_windows[i]=gtk_scrolled_window_new(NULL, NULL);

   gtk_container_add(GTK_CONTAINER(scrolled_windows[i]), sheets[i]);
   gtk_widget_show(sheets[i]);

   gtk_widget_show(scrolled_windows[i]);

   label=gtk_label_new(folder[i]);
   gtk_notebook_append_page(GTK_NOTEBOOK(notebook), GTK_WIDGET(scrolled_windows[i]),
   label);

   gtk_signal_connect(GTK_OBJECT(gtk_sheet_get_entry(GTK_SHEET(sheets[i]))),
		      "changed", (GtkSignalFunc)show_entry, NULL);

   gtk_signal_connect(GTK_OBJECT(sheets[i]),
		      "activate", (GtkSignalFunc)activate_sheet_cell,
		      NULL);
  }


  gtk_signal_connect(GTK_OBJECT(entry),
		      "changed", (GtkSignalFunc)show_sheet_entry, NULL);

  gtk_signal_connect(GTK_OBJECT(entry),
		      "activate", (GtkSignalFunc)activate_sheet_entry,
		      NULL);


  build_example1(sheets[0]); 
  build_example2(sheets[1]);
  build_example3(sheets[2]);

  gtk_signal_connect(GTK_OBJECT(gtk_sheet_get_entry(GTK_SHEET(sheets[2]))),
		     "changed", (GtkSignalFunc)show_entry, NULL);


  pixmap=gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
				    left_just);
  tpixmap = gtk_pixmap_new(pixmap, mask);
  gtk_container_add(GTK_CONTAINER(left_button), tpixmap);
  gtk_widget_show(tpixmap);
  gdk_pixmap_unref(pixmap);
  gdk_bitmap_unref(mask);

  pixmap=gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
				    center_just);
  tpixmap = gtk_pixmap_new(pixmap, mask);
  gtk_container_add(GTK_CONTAINER(center_button), tpixmap);
  gtk_widget_show(tpixmap);
  gdk_pixmap_unref(pixmap);
  gdk_bitmap_unref(mask);

  pixmap=gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
				    right_just);
  tpixmap = gtk_pixmap_new(pixmap, mask);
  gtk_container_add(GTK_CONTAINER(right_button), tpixmap);
  gtk_widget_show(tpixmap);
  gdk_pixmap_unref(pixmap);
  gdk_bitmap_unref(mask);

  pixmap=gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
				    paint);

  bg_pixmap=gtk_pixmap_new(pixmap, mask);
  gtk_container_add(GTK_CONTAINER(GTK_COMBO_BUTTON(bgcolorcombo)->button),
		    bg_pixmap);

  gtk_widget_show(bg_pixmap);
  gdk_pixmap_unref(pixmap);
  gdk_bitmap_unref(mask);

  pixmap=gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL,
				    font);


  fg_pixmap=gtk_pixmap_new(pixmap, mask);
  gtk_container_add(GTK_CONTAINER(GTK_COMBO_BUTTON(fgcolorcombo)->button),
		    fg_pixmap);

  gtk_widget_show(fg_pixmap);
  gdk_pixmap_unref(pixmap);
  gdk_bitmap_unref(mask);

  gtk_widget_show(window);

  gtk_main();

  return(0);
}


