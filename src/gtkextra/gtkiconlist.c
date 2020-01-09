/* gtkiconlist - gtkiconlist widget for gtk+
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <pango/pango.h>
#include "gtkitementry.h"
#include "gtkiconlist.h"
#include "gtkextra-marshal.h"
#include <math.h>

#define DEFAULT_ROW_SPACING 	4
#define DEFAULT_COL_SPACING 	10	
#define DEFAULT_TEXT_SPACE 	60
#define DEFAULT_ICON_BORDER 	2
#define DEFAULT_WIDTH 150
#define DEFAULT_HEIGHT 120

#define EVENTS_MASK    (GDK_EXPOSURE_MASK |		\
                       GDK_POINTER_MOTION_MASK |	\
                       GDK_POINTER_MOTION_HINT_MASK |	\
                       GDK_BUTTON_PRESS_MASK |		\
                       GDK_BUTTON_RELEASE_MASK)

/* Signals */

extern void 
_gtkextra_signal_emit(GtkObject *object, guint signal_id, ...);

enum{
      SELECT_ICON,                       
      UNSELECT_ICON,                       
      TEXT_CHANGED,                       
      ACTIVATE_ICON,                       
      DEACTIVATE_ICON,                       
      CLICK_EVENT,                       
      LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = {0};

static void gtk_icon_list_class_init 		(GtkIconListClass *class);
static void gtk_icon_list_init 			(GtkIconList *icon_list);
static void gtk_icon_list_destroy 		(GtkObject *object);

static void gtk_icon_list_size_allocate		(GtkWidget *widget,
                                                 GtkAllocation *allocation);

static void gtk_icon_list_realize		(GtkWidget *widget);
static gint gtk_icon_list_expose		(GtkWidget *widget, 
						 GdkEventExpose *event);
static gint gtk_icon_list_button_press		(GtkWidget *widget, 
						 GdkEventButton *event);
static gint deactivate_entry			(GtkIconList *iconlist);
static gint entry_in				(GtkWidget *widget,
						 GdkEventButton *event,
						 gpointer data);
static gint entry_changed 			(GtkWidget *widget, 
						 gpointer data);
static void select_icon				(GtkIconList *iconlist, 
						 GtkIconListItem *item,
						 GdkEvent *event);
static void unselect_icon			(GtkIconList *iconlist, 
						 GtkIconListItem *item,
						 GdkEvent *event);
static void unselect_all			(GtkIconList *iconlist); 
static void set_labels			        (GtkIconList *iconlist, 
					         GtkIconListItem *item, 
						 const gchar *label); 
static GtkIconListItem *get_icon_from_entry	(GtkIconList *iconlist, 
						 GtkWidget *widget);
static void reorder_icons			(GtkIconList *iconlist);
static void item_size_request			(GtkIconList *iconlist, 
                 	 			 GtkIconListItem *item,
                  				 GtkRequisition *requisition);
static void gtk_icon_list_move			(GtkIconList *iconlist, 
						 GtkIconListItem *icon, 
                   				 guint x, guint y);
static GtkIconListItem *gtk_icon_list_real_add	(GtkIconList *iconualist,
						 GdkPixmap *pixmap,
						 GdkBitmap *mask,
						 const gchar *label,
                                                 gpointer data);
static GtkIconListItem *gtk_icon_list_put 	(GtkIconList *iconlist, 
                   				 guint x, guint y, 
                   				 GdkPixmap *pixmap,
						 GdkBitmap *mask,
                   				 const gchar *label,
                                                 gpointer data);
static gint icon_key_press			(GtkWidget *widget, 
						 GdkEventKey *key, 
						 gpointer data);
static gint sort_list				(gpointer a, gpointer b);
static void pixmap_destroy                      (GtkPixmap* pixmap);


static GtkFixedClass *parent_class = NULL;

static inline guint STRING_WIDTH(GtkWidget *widget,
                                 PangoFontDescription *font, const gchar *text)
{
  PangoRectangle rect;
  PangoLayout *layout;

  layout = gtk_widget_create_pango_layout (widget, text);
  pango_layout_set_font_description (layout, font);

  pango_layout_get_pixel_extents (layout, NULL, &rect);

  g_object_unref(G_OBJECT(layout));
  return rect.width;
}


GtkType
gtk_icon_list_get_type (void)
{
  static GtkType icon_list_type = 0;

  if (!icon_list_type)
    {
      static const GtkTypeInfo icon_list_info =
      {
	"GtkIconList",
	sizeof (GtkIconList),
	sizeof (GtkIconListClass),
	(GtkClassInitFunc) gtk_icon_list_class_init,
	(GtkObjectInitFunc) gtk_icon_list_init,
	/* reserved 1*/ NULL,
        /* reserved 2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      icon_list_type = gtk_type_unique (GTK_TYPE_FIXED, &icon_list_info);
    }
  return icon_list_type;
}

static GtkIconListItem*
gtk_icon_list_item_copy (const GtkIconListItem *item)
{
  GtkIconListItem *new_item;

  g_return_val_if_fail (item != NULL, NULL);

  new_item = g_new (GtkIconListItem, 1);

  *new_item = *item;

  return new_item;
}

static void
gtk_icon_list_item_free (GtkIconListItem *item)
{
  g_return_if_fail (item != NULL);

  g_free (item);
}


GType
gtk_icon_list_item_get_type (void)
{
  static GType icon_list_item_type;

  if(!icon_list_item_type)
  {
    icon_list_item_type = g_boxed_type_register_static("GtkIconListItem", (GBoxedCopyFunc)gtk_icon_list_item_copy, (GBoxedFreeFunc)gtk_icon_list_item_free);
  }
  return icon_list_item_type;
}
 

static void
gtk_icon_list_class_init (GtkIconListClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  parent_class = gtk_type_class (GTK_TYPE_FIXED);

  object_class = (GtkObjectClass *) klass;
  widget_class = (GtkWidgetClass *) klass;

  object_class->destroy = gtk_icon_list_destroy;

  widget_class->realize = gtk_icon_list_realize;

  widget_class->size_allocate = gtk_icon_list_size_allocate;

  widget_class->expose_event = gtk_icon_list_expose;
  widget_class->button_press_event = gtk_icon_list_button_press;

  signals[SELECT_ICON] =
      gtk_signal_new("select_icon",
		     GTK_RUN_LAST,
		     GTK_CLASS_TYPE(object_class),
		     GTK_SIGNAL_OFFSET(GtkIconListClass, select_icon),
                     gtkextra_BOOLEAN__BOXED_BOXED,
		     GTK_TYPE_BOOL, 2,
		     GTK_TYPE_ICON_LIST_ITEM, GDK_TYPE_EVENT); 

  signals[UNSELECT_ICON] =
      gtk_signal_new("unselect_icon",
		     GTK_RUN_FIRST,
		     GTK_CLASS_TYPE(object_class),
		     GTK_SIGNAL_OFFSET(GtkIconListClass, unselect_icon),
                     gtkextra_VOID__BOXED_BOXED,
		     GTK_TYPE_NONE, 2,
		     GTK_TYPE_ICON_LIST_ITEM, GDK_TYPE_EVENT); 

  signals[TEXT_CHANGED] =
      gtk_signal_new("text_changed",
		     GTK_RUN_LAST,
		     GTK_CLASS_TYPE(object_class),
		     GTK_SIGNAL_OFFSET(GtkIconListClass, text_changed),
                     gtkextra_BOOLEAN__BOXED_STRING,
		     GTK_TYPE_BOOL, 2,
		     GTK_TYPE_ICON_LIST_ITEM, GTK_TYPE_STRING); 

  signals[ACTIVATE_ICON] =
      gtk_signal_new("activate_icon",
		     GTK_RUN_LAST,
		     GTK_CLASS_TYPE(object_class),
		     GTK_SIGNAL_OFFSET(GtkIconListClass, activate_icon),
                     gtkextra_BOOLEAN__BOXED,
		     GTK_TYPE_BOOL, 1,
		     GTK_TYPE_ICON_LIST_ITEM); 

  signals[DEACTIVATE_ICON] =
      gtk_signal_new("deactivate_icon",
		     GTK_RUN_LAST,
		     GTK_CLASS_TYPE(object_class),
		     GTK_SIGNAL_OFFSET(GtkIconListClass, deactivate_icon),
                     gtkextra_BOOLEAN__BOXED,
		     GTK_TYPE_BOOL, 1,
		     GTK_TYPE_ICON_LIST_ITEM); 

  signals[CLICK_EVENT] =
      gtk_signal_new("click_event",
		     GTK_RUN_LAST,
		     GTK_CLASS_TYPE(object_class),
		     GTK_SIGNAL_OFFSET(GtkIconListClass, click_event),
                     gtkextra_VOID__BOXED,
		     GTK_TYPE_NONE, 1,
		     GDK_TYPE_EVENT); 
 
}

void
gtk_icon_list_freeze(GtkIconList *iconlist)
{
 iconlist->freeze_count++;
}

void
gtk_icon_list_thaw(GtkIconList *iconlist)
{
 if(iconlist->freeze_count == 0) return;
 iconlist->freeze_count--;
 if(iconlist->freeze_count == 0)
               reorder_icons(iconlist);
}

void
gtk_icon_list_update(GtkIconList *iconlist)
{
  reorder_icons(iconlist);
}

static void
reorder_icons(GtkIconList *iconlist)
{
  GtkWidget *widget;
  GtkIconListItem *item;
  GtkRequisition req;
  GList *icons;
  gint hspace = 0;
  gint vspace = 0;
  gint x = 0;
  gint y = 0;
  gint old_width, old_height;

  widget = GTK_WIDGET(iconlist);

  if(iconlist->freeze_count > 0) return;
/*
  gdk_threads_enter();
*/

  old_width = widget->allocation.width;
  old_height = widget->allocation.height;

  y = iconlist->row_spacing;
  x = iconlist->col_spacing;

  icons = iconlist->icons;
  while(icons){
    item = (GtkIconListItem *) icons->data;

    gtk_icon_list_move(iconlist, item, x, y);

    item_size_request(iconlist, item, &req);
 
    vspace = req.height + iconlist->row_spacing;
    hspace = req.width + iconlist->col_spacing;

    switch(iconlist->mode){
      case GTK_ICON_LIST_TEXT_RIGHT:
        y += vspace;
        if(y + vspace >= old_height - DEFAULT_COL_SPACING){
                x += hspace;
                y = iconlist->row_spacing;
        }
        break;
      case GTK_ICON_LIST_TEXT_BELOW:
      case GTK_ICON_LIST_ICON:
      default:
        x += hspace;
        if(x + hspace >= old_width - DEFAULT_COL_SPACING){
                x = iconlist->col_spacing;
                y += vspace;
        }
        break;
    }

    icons = icons->next;

   }
/*
  gdk_threads_leave();
*/
}


static void
gtk_icon_list_move(GtkIconList *iconlist, GtkIconListItem *icon, 
                   guint x, guint y)
{
  GtkRequisition req1, req2;
  GtkRequisition req;
  GtkAllocation a;
  const gchar *text;
  gint old_width, old_height, width, height;
  gint old_x, old_y;
  gint size;

  old_x = icon->x;
  old_y = icon->y;

  icon->x = x;
  icon->y = y;

  item_size_request(iconlist, icon, &req);
  req1 = icon->pixmap->requisition;
  req2 = icon->entry->requisition;
  req2.width = iconlist->text_space;

  req1.width += 2*iconlist->icon_border;
  req1.height += 2*iconlist->icon_border;
  if(iconlist->mode == GTK_ICON_LIST_TEXT_BELOW){
     req1.width = MAX(req1.width, req.width);
  }

  if(iconlist->mode == GTK_ICON_LIST_ICON) 
          req2.width = req2.height = 0;

  old_width = width = GTK_WIDGET(iconlist)->allocation.width;
  old_height = height = GTK_WIDGET(iconlist)->allocation.height;

  gtk_fixed_move(GTK_FIXED(iconlist), icon->pixmap, 
                 x + req1.width/2 - icon->pixmap->requisition.width/2, 
                 y + iconlist->icon_border);

  icon->pixmap->allocation.x += (x - old_x);
  icon->pixmap->allocation.y += (y - old_y);
  icon->entry->allocation.x += (x - old_x);
  icon->entry->allocation.y += (y - old_y);
  icon->entry->allocation.width = req2.width;


  switch(iconlist->mode){
   case GTK_ICON_LIST_TEXT_BELOW:
        text = gtk_entry_get_text(GTK_ENTRY(icon->entry));
        size = STRING_WIDTH(icon->entry, icon->entry->style->font_desc, text);
        size = MIN(size, req2.width);

  	gtk_fixed_move(GTK_FIXED(iconlist), icon->entry, 
        	        x - req2.width/2 + req1.width/2, 
                        y + req1.height + iconlist->icon_border);

        if(y + req.height > height) 
           height += req.height;
        break;
   case GTK_ICON_LIST_TEXT_RIGHT:
  	gtk_fixed_move(GTK_FIXED(iconlist), icon->entry, 
        	        x + req1.width + iconlist->icon_border, 
                        y + req1.height/2 - req2.height/2); 

        if(x + req.width > width) 
            width += req.width;
	break;
   case GTK_ICON_LIST_ICON:
   default: ;
  }

  a = icon->entry->allocation;

  gtk_widget_size_allocate(icon->pixmap, &icon->pixmap->allocation);
  if(icon->entry){
    gtk_widget_size_allocate(icon->entry, &a);
    gtk_widget_draw(icon->entry, NULL);
  }

}


static void
gtk_icon_list_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
  GtkAllocation old = widget->allocation;
  GTK_WIDGET_CLASS(parent_class)->size_allocate(widget, allocation);
  if(old.width != allocation->width || old.height != allocation->height)
    reorder_icons(GTK_ICON_LIST(widget));
}


static void
gtk_icon_list_realize(GtkWidget *widget)
{
  GList *icons;
  GtkIconList *iconlist;
  GtkIconListItem *item;
  GtkStyle *style;
  
  GTK_WIDGET_CLASS(parent_class)->realize (widget);

  iconlist = GTK_ICON_LIST(widget);

  style = gtk_style_copy(widget->style);

  style->bg[0] = iconlist->background;
  gtk_widget_set_style(widget, style);
  gtk_style_set_background(style, widget->window, GTK_STATE_NORMAL);
  gtk_style_set_background(style, widget->window, GTK_STATE_ACTIVE);

  icons = iconlist->icons;
  while(icons){
    item = (GtkIconListItem *) icons->data;
    gtk_widget_draw(item->pixmap, NULL);
    if(iconlist->mode != GTK_ICON_LIST_ICON){
      GtkStyle *style;

      gtk_widget_realize(item->entry);

      style = gtk_style_copy(item->entry->style);
      style->bg[GTK_STATE_ACTIVE] = iconlist->background;
      style->bg[GTK_STATE_NORMAL] = iconlist->background;
      gtk_widget_set_style(item->entry, style);
      gtk_widget_show(item->entry);
    }
    if(item->entry) gtk_widget_draw(item->entry, NULL);
    icons = icons->next;
  }

  reorder_icons(iconlist);
}


static void
gtk_icon_list_init (GtkIconList *icon_list)
{
  GtkWidget *widget;
  widget = GTK_WIDGET(icon_list);

  gtk_widget_ensure_style(widget);
  gdk_color_black(gtk_widget_get_colormap(widget), &widget->style->black);
  gdk_color_white(gtk_widget_get_colormap(widget), &widget->style->white);

  gtk_fixed_set_has_window(GTK_FIXED(widget), TRUE);

  gtk_widget_set_events (widget, gtk_widget_get_events(widget)|
                         EVENTS_MASK);

  icon_list->selection = NULL;
  icon_list->is_editable = TRUE;

  icon_list->num_icons = 0;
  icon_list->background = widget->style->white;

  icon_list->row_spacing = DEFAULT_ROW_SPACING;
  icon_list->col_spacing = DEFAULT_COL_SPACING;
  icon_list->text_space = DEFAULT_TEXT_SPACE;
  icon_list->icon_border = DEFAULT_ICON_BORDER;

  icon_list->active_icon = NULL;
  icon_list->compare_func = (GCompareFunc)sort_list;
}

static gint 
sort_list(gpointer a, gpointer b)
{
  GtkIconListItem *itema;
  GtkIconListItem *itemb;

  itema = (GtkIconListItem *)a;
  itemb = (GtkIconListItem *)b;

  return (strcmp(itema->label, itemb->label));
}

static gboolean
gtk_icon_list_expose (GtkWidget *widget, GdkEventExpose *event)
{
  GtkIconList *icon_list;

  icon_list = GTK_ICON_LIST(widget);

  if(!GTK_WIDGET_DRAWABLE(widget)) return FALSE;

  gtk_paint_flat_box (widget->style, widget->window, GTK_STATE_NORMAL,
                      GTK_SHADOW_NONE, &event->area, widget, "base", 0, 0, -1, -1);

  GTK_WIDGET_CLASS(parent_class)->expose_event(widget, event);

  if(icon_list->active_icon && icon_list->active_icon->entry){
	gdk_draw_rectangle(widget->window,
                           widget->style->black_gc,
                           FALSE,
                           icon_list->active_icon->entry->allocation.x-2,
                           icon_list->active_icon->entry->allocation.y-2,
                           icon_list->active_icon->entry->allocation.width+4,
                           icon_list->active_icon->entry->allocation.height+4);
  }

  return FALSE;
}

static gint
gtk_icon_list_button_press(GtkWidget *widget, GdkEventButton *event)
{
  GtkIconList *iconlist;
  GtkIconListItem *item;
  gint x, y;
  GtkAllocation *allocation;

  if(!GTK_IS_ICON_LIST(widget)) return FALSE;

  iconlist = GTK_ICON_LIST(widget);


  gtk_widget_get_pointer(widget, &x, &y);
  item = gtk_icon_list_get_icon_at(iconlist, x , y );

  if(!item){ 
     gtk_signal_emit(GTK_OBJECT(iconlist), signals[CLICK_EVENT],
                     event);
     return FALSE;
  }

  if(item->entry){
    allocation = &item->entry->allocation;
    if(x >= allocation->x &&
       x <= allocation->x + allocation->width &&
       y >= allocation->y &&
       y <= allocation->y + allocation->height) return FALSE;
  }

  if(item)
   switch(iconlist->selection_mode){
     case GTK_SELECTION_SINGLE:
     case GTK_SELECTION_BROWSE:
        unselect_all(iconlist);
     case GTK_SELECTION_MULTIPLE:
        select_icon(iconlist, item, (GdkEvent *)event);
     case GTK_SELECTION_NONE:
        break;
   }

  return FALSE;
}


static gint 
deactivate_entry(GtkIconList *iconlist)
{
  GdkGC *gc;
  GtkEntry *entry;
  gboolean veto = TRUE;

  if(iconlist->active_icon) {
     _gtkextra_signal_emit(GTK_OBJECT(iconlist), signals[DEACTIVATE_ICON], 
                     iconlist->active_icon, &veto);
     if(!veto) return FALSE;

     entry = GTK_ENTRY(iconlist->active_icon->entry);

     if(!entry || !GTK_WIDGET_REALIZED(entry)) return TRUE;
     gtk_entry_set_editable(entry, FALSE);    
     gtk_entry_select_region(entry, 0, 0);
     gtk_item_entry_set_cursor_visible(GTK_ITEM_ENTRY(entry), FALSE);

     switch(iconlist->mode){
       case GTK_ICON_LIST_TEXT_RIGHT:
         gtk_item_entry_set_text(GTK_ITEM_ENTRY(entry), 
                                 iconlist->active_icon->entry_label,
                                 GTK_JUSTIFY_LEFT);
         break;
       case GTK_ICON_LIST_TEXT_BELOW:
         gtk_item_entry_set_text(GTK_ITEM_ENTRY(entry), 
                                 iconlist->active_icon->entry_label,
                                 GTK_JUSTIFY_CENTER);
         break;
       case GTK_ICON_LIST_ICON:
       default:
         break;
     }

     if(GTK_WIDGET_REALIZED(iconlist->active_icon->entry)){
       gc = gdk_gc_new(GTK_WIDGET(iconlist)->window);
       gdk_gc_set_foreground(gc, &iconlist->background);
       gdk_draw_rectangle(GTK_WIDGET(iconlist)->window,
                          gc,
                          FALSE,
                          GTK_WIDGET(entry)->allocation.x-2,
                          GTK_WIDGET(entry)->allocation.y-2,
                          GTK_WIDGET(entry)->allocation.width+4,
                          GTK_WIDGET(entry)->allocation.height+4);
       gdk_gc_unref(gc);
     }

     iconlist->active_icon->state = GTK_STATE_NORMAL;
     iconlist->active_icon = NULL;
  }

  return TRUE;
}


static void
select_icon(GtkIconList *iconlist, GtkIconListItem *item, GdkEvent *event)
{
  gboolean veto = TRUE;

  if(item == NULL) return;

  _gtkextra_signal_emit(GTK_OBJECT(iconlist), signals[SELECT_ICON], item, 
                  event, &veto);                        

  if(!veto) return;

  if(iconlist->mode != GTK_ICON_LIST_ICON){ 
    if(!deactivate_entry(iconlist)) return;

    if(item->entry && GTK_WIDGET_REALIZED(item->entry)){
      GtkStyle *style = gtk_style_copy(item->entry->style);
      style->bg[GTK_STATE_ACTIVE] = style->base[GTK_STATE_SELECTED];
      style->bg[GTK_STATE_NORMAL] = style->base[GTK_STATE_SELECTED];
      style->text[GTK_STATE_ACTIVE] = style->text[GTK_STATE_SELECTED];
      style->text[GTK_STATE_NORMAL] = style->text[GTK_STATE_SELECTED];
      gtk_widget_set_style(item->entry, style);
      gtk_style_unref(style);

      switch(iconlist->mode){
        case GTK_ICON_LIST_TEXT_RIGHT:
          gtk_item_entry_set_text(GTK_ITEM_ENTRY(item->entry), 
                                  item->label,
                                  GTK_JUSTIFY_LEFT);
          break;
        case GTK_ICON_LIST_TEXT_BELOW:
          gtk_item_entry_set_text(GTK_ITEM_ENTRY(item->entry), 
                                  item->label,
                                  GTK_JUSTIFY_CENTER);
          break;
        case GTK_ICON_LIST_ICON:
        default:
          break;
      }
    }
  }
  if(item->state == GTK_STATE_SELECTED) return;
  iconlist->selection = g_list_append(iconlist->selection, item);

  item->state = GTK_STATE_SELECTED;  
  if(item->entry) gtk_widget_grab_focus(item->entry);
}

static void
unselect_icon(GtkIconList *iconlist, GtkIconListItem *item, GdkEvent *event)
{
  GList *selection;
  GtkIconListItem *icon;

  if(!item) return;

  if(item->state == GTK_STATE_NORMAL) return;

  selection = iconlist->selection;
  while(selection){
    icon = (GtkIconListItem *)selection->data; 
    if(item == icon) break;
    selection = selection->next;
  }

  if(selection){
       iconlist->selection = g_list_remove_link(iconlist->selection, selection);
  }
 
  item->state = GTK_STATE_NORMAL;

  if(iconlist->mode != GTK_ICON_LIST_ICON){
   if(item->entry && GTK_WIDGET_REALIZED(item->entry)){
     GtkStyle *style = gtk_style_copy(item->entry->style);

     style->bg[GTK_STATE_ACTIVE] = iconlist->background;
     style->bg[GTK_STATE_NORMAL] = iconlist->background;
     style->text[GTK_STATE_ACTIVE] = GTK_WIDGET(iconlist)->style->text[GTK_STATE_ACTIVE];
     style->text[GTK_STATE_NORMAL] = GTK_WIDGET(iconlist)->style->text[GTK_STATE_NORMAL];
     gtk_widget_set_style(item->entry, style);
     gtk_style_unref(style);

     gtk_entry_select_region(GTK_ENTRY(item->entry), 0, 0);
     gtk_entry_set_text(GTK_ENTRY(item->entry), item->entry_label);
     gtk_entry_set_editable(GTK_ENTRY(item->entry), FALSE);
     gtk_widget_draw(item->entry, NULL);

   }
  }

  gtk_signal_emit(GTK_OBJECT(iconlist), signals[UNSELECT_ICON], item, event);                        
}

static void
unselect_all(GtkIconList *iconlist)
{
  GList *selection;
  GtkIconListItem *item;

  selection = iconlist->selection;
  while(selection){
    item = (GtkIconListItem *)selection->data;
    unselect_icon(iconlist, item, NULL);
    selection = iconlist->selection;
  }

  g_list_free(iconlist->selection);
  iconlist->selection = NULL;
}

GtkWidget*
gtk_icon_list_new (guint icon_width, GtkIconListMode mode)
{
  GtkIconList *icon_list;

  icon_list = gtk_type_new (gtk_icon_list_get_type ());

  gtk_icon_list_construct(icon_list, icon_width, mode);

  return GTK_WIDGET (icon_list);
}

void
gtk_icon_list_construct (GtkIconList *icon_list, guint icon_width, GtkIconListMode mode)
{
  icon_list->icon_width = icon_width;
  icon_list->mode = mode;
  icon_list->icons = NULL;
  icon_list->selection = NULL;
  icon_list->selection_mode = GTK_SELECTION_SINGLE;
}

void
gtk_icon_list_set_mode (GtkIconList *iconlist, GtkIconListMode mode)
{
  GList *icons;

  iconlist->mode = mode;

  icons = iconlist->icons;
  while(icons){
    GtkIconListItem *item;

    item = (GtkIconListItem *)icons->data;

    switch(mode){
      case GTK_ICON_LIST_TEXT_RIGHT:
        gtk_item_entry_set_justification(GTK_ITEM_ENTRY(item->entry), 
                                         GTK_JUSTIFY_LEFT);
        break;
      case GTK_ICON_LIST_TEXT_BELOW:
        gtk_item_entry_set_justification(GTK_ITEM_ENTRY(item->entry), 
                                         GTK_JUSTIFY_CENTER);
        break;
      case GTK_ICON_LIST_ICON:
      default:
        break;
    }
    
    icons = icons->next;
  }
 
  reorder_icons(iconlist); 
}

GtkIconListMode
gtk_icon_list_get_mode (GtkIconList *iconlist)
{
  return(iconlist->mode);
}

void
gtk_icon_list_set_text_space (GtkIconList *iconlist, guint text_space)
{
  GList *icons;

  iconlist->text_space = text_space;

  icons = iconlist->icons;
  while(icons){
    GtkIconListItem *item;

    item = (GtkIconListItem *)icons->data;
    
    if(item->entry) GTK_ITEM_ENTRY(item->entry)->text_max_size = text_space;
    
    icons = icons->next;
  }
 
  reorder_icons(iconlist); 
}


static void
gtk_icon_list_destroy (GtkObject *object)
{
  GtkIconList *icon_list;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_ICON_LIST (object));

  icon_list = GTK_ICON_LIST (object);

  gtk_icon_list_clear(icon_list);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


void
gtk_icon_list_set_background (GtkIconList *iconlist, GdkColor *color)
{
  GtkWidget *widget;
  GtkStyle *style;
  
  g_return_if_fail (iconlist != NULL);
  g_return_if_fail (GTK_IS_ICON_LIST (iconlist));

  widget = GTK_WIDGET(iconlist);

  iconlist->background = *color;

  style = gtk_style_copy(widget->style);
  style->bg[0] = iconlist->background;

  gtk_widget_set_style(widget, style);
  if(widget->window) gdk_window_set_background(widget->window, color);
  gtk_style_unref(style);

}

static gint
entry_changed (GtkWidget *widget, gpointer data)
{
  GtkIconList *iconlist;
  GtkIconListItem *item;
  gboolean veto = TRUE;
  const gchar *text;

  iconlist = GTK_ICON_LIST(data);
  item = get_icon_from_entry(iconlist, widget);
  text = gtk_entry_get_text(GTK_ENTRY(widget));

  _gtkextra_signal_emit(GTK_OBJECT(data), signals[TEXT_CHANGED],
                  item, text, &veto);

  if(!veto) return veto; 

  if(item->entry && gtk_editable_get_editable(GTK_EDITABLE(item->entry))){
    if(item->label) g_free(item->label);
    if(text) item->label = g_strdup(text); 
    if(item->entry_label) g_free(item->entry_label);
    set_labels(iconlist, item, text); 
  }

  return veto;
} 
                  
static gint
entry_in (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  GtkIconList *iconlist;
  GtkIconListItem *item;
  gboolean veto = TRUE;

  if(!GTK_IS_ENTRY(widget)) return FALSE;
  iconlist = GTK_ICON_LIST(data);

  item = get_icon_from_entry(iconlist, widget);
  if(iconlist->active_icon && iconlist->active_icon->entry == widget) 
          return FALSE;

  _gtkextra_signal_emit(GTK_OBJECT(iconlist), signals[ACTIVATE_ICON], &item, &veto);

  if(!veto) return FALSE; 
  if(!deactivate_entry(iconlist)) return FALSE;

  if(item->state == GTK_STATE_SELECTED){
   if(iconlist->is_editable && !gtk_editable_get_editable(GTK_EDITABLE(widget))){
     unselect_all(iconlist);

     gtk_entry_set_editable(GTK_ENTRY(widget), TRUE);
     gtk_item_entry_set_cursor_visible(GTK_ITEM_ENTRY(widget), TRUE);
     if(item->label) gtk_entry_set_text(GTK_ENTRY(widget), item->label);
     iconlist->active_icon = item;
     item->state = GTK_STATE_NORMAL;

     if(GTK_WIDGET_DRAWABLE(widget)) 
       gdk_draw_rectangle(GTK_WIDGET(iconlist)->window,
                          widget->style->black_gc,
                          FALSE,
                          iconlist->active_icon->entry->allocation.x-2,
                          iconlist->active_icon->entry->allocation.y-2,
                          iconlist->active_icon->entry->allocation.width+4,
                          iconlist->active_icon->entry->allocation.height+4);
   }else{
     gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "button_press_event"); 
     if(iconlist->selection_mode == GTK_SELECTION_SINGLE ||
        iconlist->selection_mode == GTK_SELECTION_BROWSE) 
          unselect_all(iconlist);
     select_icon(iconlist, item, (GdkEvent *)event);
   }
  }else{
     if(iconlist->selection_mode == GTK_SELECTION_SINGLE ||
        iconlist->selection_mode == GTK_SELECTION_BROWSE) 
          unselect_all(iconlist);
     select_icon(iconlist, item, (GdkEvent *)event);
  }

  return FALSE;
}

GtkIconListItem *
gtk_icon_list_get_active_icon(GtkIconList *iconlist)
{
  return iconlist->active_icon;
}

static GtkIconListItem *
get_icon_from_entry(GtkIconList *iconlist, GtkWidget *widget)
{
  GList *icons;
  GtkIconListItem *item;

  icons = iconlist->icons;
  while(icons){
    item = (GtkIconListItem *) icons->data;
    if(widget == item->entry) return item;
    icons = icons->next;
  }

  return NULL;
}

GtkIconListItem *
gtk_icon_list_get_icon_at(GtkIconList *iconlist, gint x, gint y)
{
  GList *icons;
  GtkIconListItem *item;
  GtkRequisition req;

  icons = iconlist->icons;
  while(icons){
    item = (GtkIconListItem *) icons->data;
    item_size_request(iconlist, item, &req);
    if(x >= item->x && x <= item->x + req.width &&
       y >= item->y && y <= item->y + req.height) return item;
    icons = icons->next;
  }

  return NULL;
}

GtkIconListItem *
gtk_icon_list_add (GtkIconList *iconlist, 
                   const gchar *file,
                   const gchar *label,
                   gpointer link)
{
  GtkIconListItem *item;
  GdkColormap *colormap;
  GdkPixmap *pixmap;
  GdkBitmap *mask;

  colormap = gdk_colormap_get_system();
  pixmap = gdk_pixmap_colormap_create_from_xpm(NULL, colormap, &mask, NULL, 
                                               file);
  item = gtk_icon_list_real_add(iconlist, pixmap, mask, label, link);
  return item;
}

GtkIconListItem *
gtk_icon_list_add_from_data (GtkIconList *iconlist, 
                             gchar **data,
                             const gchar *label,
                             gpointer link)
{
  GtkIconListItem *item;
  GdkColormap *colormap;
  GdkPixmap *pixmap;
  GdkBitmap *mask;

  colormap = gdk_colormap_get_system();
  pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL, colormap, &mask, NULL, 
                                                 data);
  item = gtk_icon_list_real_add(iconlist, pixmap, mask, label, link);
  return item;
}

GtkIconListItem *
gtk_icon_list_add_from_pixmap (GtkIconList *iconlist, 
                               GdkPixmap *pixmap,
                               GdkBitmap *mask,
                               const gchar *label,
                               gpointer link)
{
  GtkIconListItem *item;

  gdk_pixmap_ref(pixmap);
  if(mask) gdk_bitmap_ref(mask);
  item = gtk_icon_list_real_add(iconlist, pixmap, mask, label, link);
  return item;
}


static GtkIconListItem*
gtk_icon_list_real_add (GtkIconList *iconlist, 
                        GdkPixmap *pixmap,
                        GdkBitmap *mask,
                        const gchar *label,
                        gpointer data)
{
  GtkIconListItem *item;
  GtkRequisition requisition;
  gint hspace = 0;
  gint vspace = 0;
  gint x = 0;
  gint y = 0;
  gint width, height;

  width = GTK_WIDGET(iconlist)->allocation.width;
  height = GTK_WIDGET(iconlist)->allocation.height;

  if(iconlist->num_icons > 0){
    item = gtk_icon_list_get_nth(iconlist, iconlist->num_icons-1);
    x = item->x;
    y = item->y;
    item_size_request(iconlist, item, &requisition);
 
    vspace = requisition.height + iconlist->row_spacing;
    hspace = requisition.width + iconlist->col_spacing;

    switch(iconlist->mode){
      case GTK_ICON_LIST_TEXT_RIGHT:
        y += vspace;
        if(y >= height){
                x += hspace;
                y = iconlist->row_spacing;
        }
        break;
      case GTK_ICON_LIST_TEXT_BELOW:
      case GTK_ICON_LIST_ICON:
      default:
        x += hspace;
        if(x >= width){
                x = iconlist->col_spacing;
                y += vspace;
        }
        break;
    }
  } else {
    y = iconlist->row_spacing;
    x = iconlist->col_spacing;
  }

  item = gtk_icon_list_put(iconlist, x, y, pixmap, mask, label, data);
  return item;
}

static GtkIconListItem *
gtk_icon_list_put (GtkIconList *iconlist, 
                   guint x, guint y, 
                   GdkPixmap *pixmap,
                   GdkBitmap *mask,
                   const gchar *label,
                   gpointer data)
{
  GtkIconListItem *icon;
  GtkIconListItem *active_icon;
  GtkWidget *widget;
  GtkRequisition req, req1, req2;
  gint text_width;
  gint width, height, old_width, old_height;
  GtkAllocation alloc;

  widget = GTK_WIDGET(iconlist);

  old_width = width = GTK_WIDGET(iconlist)->allocation.width;
  old_height = height = GTK_WIDGET(iconlist)->allocation.height;

  active_icon = iconlist->active_icon;
  gtk_icon_list_set_active_icon(iconlist, NULL);

  icon = g_new(GtkIconListItem, 1);
  icon->x = x;
  icon->y = y;
  icon->state = GTK_STATE_NORMAL;
  icon->label = NULL;
  icon->entry_label = NULL;
  if(label) icon->label = g_strdup(label);
  icon->entry = gtk_item_entry_new();
  icon->pixmap = gtk_pixmap_new(pixmap, mask);
  icon->link = data;

  GTK_ITEM_ENTRY(icon->entry)->text_max_size = iconlist->text_space; 
  item_size_request(iconlist, icon, &req);
  req1 = icon->pixmap->requisition;
  req2 = icon->entry->requisition;
  req2.width = iconlist->text_space; 

  req1.width += 2*iconlist->icon_border;
  req1.height += 2*iconlist->icon_border;
  if(iconlist->mode == GTK_ICON_LIST_TEXT_BELOW){
     req1.width = MAX(req1.width, req.width);
  }

  if(iconlist->mode == GTK_ICON_LIST_ICON) 
          req2.width = req2.height = 0;
  else
          set_labels(iconlist, icon, label);

  text_width = 0;
  if(label) text_width = STRING_WIDTH(icon->entry, icon->entry->style->font_desc, label);

  gtk_fixed_put(GTK_FIXED(iconlist), icon->pixmap, 
                 x + req1.width/2 - icon->pixmap->requisition.width/2, 
                 y + iconlist->icon_border);

  alloc.x = x + req1.width/2 - icon->pixmap->requisition.width/2; 
  alloc.y = y + iconlist->icon_border; 
  alloc.width =  req1.width;
  alloc.height =  req1.height;
  gtk_widget_size_allocate(icon->pixmap, &alloc);

  switch(iconlist->mode){
   case GTK_ICON_LIST_TEXT_BELOW:
        gtk_item_entry_set_text(GTK_ITEM_ENTRY(icon->entry), icon->entry_label, 
                                GTK_JUSTIFY_CENTER);
  	gtk_fixed_put(GTK_FIXED(iconlist), icon->entry, 
        	       x - req2.width/2 + req1.width/2, 
                       y + req1.height + iconlist->icon_border);
        alloc.x = x - req2.width/2 + req1.width/2; 
        alloc.y = y + req1.height + iconlist->icon_border;
        alloc.width = req2.width;
        alloc.height = req2.height;
        gtk_widget_size_allocate(icon->entry, &alloc);

        if(y + req1.height + iconlist->icon_border + req2.height > height) 
           height += req1.height + iconlist->icon_border + req2.height;
        break;
   case GTK_ICON_LIST_TEXT_RIGHT:
        gtk_item_entry_set_text(GTK_ITEM_ENTRY(icon->entry), icon->entry_label, 
                                GTK_JUSTIFY_LEFT);
  	gtk_fixed_put(GTK_FIXED(iconlist), icon->entry, 
        	       x + req1.width + iconlist->icon_border, 
                       y + req1.height/2 - req2.height/2); 
        alloc.x = x + req1.width + iconlist->icon_border; 
        alloc.y = y + req1.height/2 - req2.height/2; 
        alloc.width = req2.width;
        alloc.height = req2.height;
        gtk_widget_size_allocate(icon->entry, &alloc);

        if(x + req1.width + iconlist->icon_border + text_width > width) 
            width += req1.width + iconlist->icon_border + text_width;
	break;
   case GTK_ICON_LIST_ICON:
   default: ;
  }

  if(GTK_WIDGET_REALIZED(iconlist))
    if(iconlist->mode != GTK_ICON_LIST_ICON){
      GtkStyle *style = gtk_style_copy(icon->entry->style);
      style->bg[GTK_STATE_ACTIVE] = iconlist->background;
      style->bg[GTK_STATE_NORMAL] = iconlist->background;
      gtk_widget_set_style(icon->entry, style);
      gtk_style_unref(style);
      gtk_widget_show(icon->entry);
    }

  gtk_widget_show(icon->pixmap);

  if(iconlist->compare_func)
    iconlist->icons = g_list_insert_sorted(iconlist->icons, icon, iconlist->compare_func);
  else
    iconlist->icons = g_list_append(iconlist->icons, icon);

  iconlist->num_icons++;

  if(GTK_WIDGET_REALIZED(iconlist))
                reorder_icons(iconlist);

  gtk_entry_set_editable(GTK_ENTRY(icon->entry), FALSE);

  gtk_signal_connect(GTK_OBJECT(icon->entry), "key_press_event",
                    (GtkSignalFunc)icon_key_press, iconlist);
  gtk_signal_connect(GTK_OBJECT(icon->entry), "button_press_event", 
                     (GtkSignalFunc)entry_in, iconlist);
  gtk_signal_connect(GTK_OBJECT(icon->entry), "changed", 
                     (GtkSignalFunc)entry_changed, iconlist);

  gtk_icon_list_set_active_icon(iconlist, active_icon);
  return icon;
}

static void 
set_labels(GtkIconList *iconlist, GtkIconListItem *icon, const gchar *label)
{
  gint text_width;
  gint point_width;
  gint max_width;
  gchar *entry_label = NULL;
  gint n, space;

  if(!label) return;

  entry_label = (gchar *)g_malloc(strlen(label) + 5);
  entry_label[0] = label[0];
  entry_label[1] = '\0';

  text_width = STRING_WIDTH(icon->entry, icon->entry->style->font_desc, label);
  point_width = STRING_WIDTH(icon->entry, icon->entry->style->font_desc, "X");

  max_width = iconlist->text_space;

  for(n = 0; n < strlen(label); n++){
     space = strlen(label) - n + 1;
     if(space > 3 && 
        STRING_WIDTH(icon->entry, icon->entry->style->font_desc, entry_label) +
        3 * point_width > max_width) 
                                                   break;
     entry_label[n] = label[n];
     entry_label[n + 1] = '\0';
  }


  if(strlen(entry_label) < strlen(label))
      sprintf(entry_label,"%s...", entry_label);

  icon->entry_label = g_strdup(entry_label);

  g_free(entry_label);
}

static gint
icon_key_press(GtkWidget *widget, GdkEventKey *key, gpointer data)
{
  GtkIconList *iconlist;

  iconlist = GTK_ICON_LIST(data);
  if(key->keyval != GDK_Return) return FALSE;

  if(iconlist->active_icon)
          select_icon(iconlist, iconlist->active_icon, NULL);

  return FALSE;
}

static void
item_size_request(GtkIconList *iconlist, 
                  GtkIconListItem *item,
                  GtkRequisition *requisition)
{
  GtkRequisition req2;

  gtk_widget_size_request(item->entry, &req2);
  req2.width = iconlist->text_space;

  gtk_widget_size_request(item->pixmap, requisition);
  requisition->width = MAX(iconlist->icon_width, requisition->width);
  requisition->width += 2*iconlist->icon_border;
  requisition->height += 2*iconlist->icon_border;

  switch(iconlist->mode){
   case GTK_ICON_LIST_TEXT_BELOW:
        requisition->height += req2.height;
        requisition->width = MAX(requisition->width, req2.width);
        break;
   case GTK_ICON_LIST_TEXT_RIGHT:
        requisition->width += req2.width;
        break;
   case GTK_ICON_LIST_ICON:
   default: ;
  }

}

   		  
void
gtk_icon_list_set_editable (GtkIconList *iconlist, gboolean editable)
{
  GList *icons;
  GtkIconListItem *item;
  
  icons = iconlist->icons;
  while(icons){
    item = (GtkIconListItem *) icons->data;
    gtk_entry_set_editable(GTK_ENTRY(item->entry), editable);
    icons = icons->next;
  }

  iconlist->is_editable = editable;
} 

GtkIconListItem *
gtk_icon_list_get_nth(GtkIconList *iconlist, guint n)
{
  return (GtkIconListItem *)g_list_nth_data(iconlist->icons, n);
}

gint
gtk_icon_list_get_index(GtkIconList *iconlist, GtkIconListItem *item)
{
  GList *icons;
  GtkIconListItem *icon;
  gint n = 0;

  if(item == NULL) return -1;
 
  icons = iconlist->icons;
  while(icons){
    n++;
    icon = (GtkIconListItem *) icons->data;
    if(item == icon) break;
    icons = icons->next;
  }

  if(icons) return n;

  return -1;
}

static void
remove_from_fixed(GtkIconList *iconlist, GtkWidget *widget)
{
  GtkFixed *fixed;
  GList *children;

  fixed = GTK_FIXED(iconlist);
  children = fixed->children;
  while(children){
    GtkFixedChild *child;

    child = children->data;

    if(child->widget == widget){
      gtk_widget_unparent(widget);
      fixed->children = g_list_remove_link (fixed->children, children);
      g_list_free (children);
      g_free (child);

      break;
    }

    children = children->next;
  }
}

static void
pixmap_destroy(GtkPixmap* pixmap)
{
  /* release pixmap */
  if (pixmap){
    GdkPixmap* pm = NULL;
    GdkBitmap* bm = NULL;

    gtk_pixmap_get(pixmap, &pm, &bm);

    /* HB: i don't know enough about Gtk+ to call this a design flaw, but it
     * appears the pixmaps need to be destroyed by hand ...
     */
    if (pm) gdk_pixmap_unref(pm);
    if (bm) gdk_pixmap_unref(bm);
  }
}
  
void
gtk_icon_list_remove (GtkIconList *iconlist, GtkIconListItem *item)
{
  GList *icons;
  GtkIconListItem *icon = 0;

  if(item == NULL) return;
 
  icons = iconlist->icons;
  while(icons){
    icon = (GtkIconListItem *) icons->data;
    if(item == icon) break;
    icons = icons->next;
  }

  if(icons){
     if(icon->state == GTK_STATE_SELECTED) unselect_icon(iconlist, icon, NULL);
     if(icon == iconlist->active_icon) deactivate_entry(iconlist);
     pixmap_destroy(GTK_PIXMAP(icon->pixmap));
     if(icon->entry && iconlist->mode != GTK_ICON_LIST_ICON){
       remove_from_fixed(iconlist, icon->entry);
       icon->entry = NULL;
     }
     if(icon->pixmap){
       remove_from_fixed(iconlist, icon->pixmap);
       icon->pixmap = NULL;
     }
     if(icon->label){
        g_free(icon->label);
        icon->label = NULL;
     }
     if(icon->entry_label){
        g_free(icon->entry_label);
        icon->entry_label = NULL;
     }

     g_free(icon);
     iconlist->icons = g_list_remove_link(iconlist->icons, icons);
     g_list_free_1(icons);
     iconlist->num_icons--;
  }

  if(iconlist->num_icons == 0){
      iconlist->icons = NULL;
      iconlist->selection = NULL;
  }
}

void
gtk_icon_list_remove_nth (GtkIconList *iconlist, guint n)
{
  GtkIconListItem *item;

  item = gtk_icon_list_get_nth(iconlist, n);
  gtk_icon_list_remove(iconlist, item);
}

void
gtk_icon_list_clear(GtkIconList *iconlist)
{
  GList *icons;
  GtkIconListItem *icon;

  if(iconlist->num_icons == 0) return;
  if(!deactivate_entry(iconlist)) return;

  unselect_all(iconlist);

  icons = iconlist->icons;

  while(icons){
    icon = (GtkIconListItem *) icons->data;
    pixmap_destroy(GTK_PIXMAP(icon->pixmap));
    if(icon->entry && iconlist->mode != GTK_ICON_LIST_ICON){
      remove_from_fixed(iconlist, icon->entry);
      icon->entry = NULL;
    }
    if(icon->pixmap){
      gtk_widget_hide(icon->pixmap);
      remove_from_fixed(iconlist, icon->pixmap);
      icon->pixmap = NULL;
    }
    if(icon->label){
        g_free(icon->label);
        icon->label = NULL;
    }
    if(icon->entry_label){
        g_free(icon->entry_label);
        icon->entry_label = NULL;
    }

    g_free(icon);
    icon = NULL;

    iconlist->icons = g_list_remove_link(iconlist->icons, icons);
    g_list_free_1(icons);
    icons = iconlist->icons;
  }

  iconlist->icons = NULL;
  iconlist->selection = NULL;
  iconlist->active_icon = NULL;
  iconlist->num_icons = 0;
}    

void
gtk_icon_list_link(GtkIconListItem *item, gpointer data)
{
  item->link = data;
} 

gpointer
gtk_icon_list_get_link(GtkIconListItem *item)
{
  return item->link;
}

GtkIconListItem *
gtk_icon_list_get_icon_from_link(GtkIconList *iconlist, gpointer data)
{
  GList *icons;
  GtkIconListItem *item;

  icons = iconlist->icons;
  while(icons){
    item = (GtkIconListItem *) icons->data;
    if(data == item->link) return item;
    icons = icons->next;
  }

  return NULL;
}

GtkWidget *
gtk_icon_list_get_entry(GtkIconListItem *item)
{
  return item->entry;
}

GtkWidget *
gtk_icon_list_get_pixmap(GtkIconListItem *item)
{
  return item->pixmap;
}

void
gtk_icon_list_set_pixmap(GtkIconListItem *item, 
			 GdkPixmap *pixmap, 
			 GdkBitmap *mask)
{

  if(item->pixmap) gtk_widget_destroy(item->pixmap);  
  item->pixmap = gtk_pixmap_new(pixmap, mask);

}

void 
gtk_icon_list_set_label(GtkIconList *iconlist, GtkIconListItem *item, const gchar *label)
{
  if(item->label){
      g_free(item->label);
      item->label = NULL;
  }
  if(item->entry_label){
      g_free(item->entry_label);
      item->entry_label = NULL;
  }
  if(label) item->label = g_strdup(label);
  gtk_entry_set_text(GTK_ENTRY(item->entry), label);
  set_labels(iconlist, item, label);
}
 
/**********************************
 * gtk_icon_list_set_icon_width
 * gtk_icon_list_set_row_spacing
 * gtk_icon_list_set_col_spacing
 * gtk_icon_list_set_text_space
 * gtk_icon_list_icon_border
 **********************************/

void
gtk_icon_list_set_icon_width(GtkIconList *iconlist, guint width)
{
  iconlist->icon_width = width;
  reorder_icons(iconlist);
}

void
gtk_icon_list_set_icon_border(GtkIconList *iconlist, guint border)
{
  iconlist->icon_border = border;
  reorder_icons(iconlist);
}

void
gtk_icon_list_set_row_spacing(GtkIconList *iconlist, guint spacing)
{
  iconlist->row_spacing = spacing;
  reorder_icons(iconlist);
}

void
gtk_icon_list_set_col_spacing(GtkIconList *iconlist, guint spacing)
{
  iconlist->col_spacing = spacing;
  reorder_icons(iconlist);
}


/**********************************
 * gtk_icon_list_set_selection_mode
 * gtk_icon_list_select_icon
 * gtk_icon_list_unselect_icon
 * gtk_icon_list_unselect_all
 **********************************/

void
gtk_icon_list_set_selection_mode(GtkIconList *iconlist, gint mode)
{
  iconlist->selection_mode = mode;
}

void
gtk_icon_list_select_icon(GtkIconList *iconlist, GtkIconListItem *item)
{
  select_icon(iconlist, item, NULL);
}

void
gtk_icon_list_unselect_icon(GtkIconList *iconlist, GtkIconListItem *item)
{
  unselect_icon(iconlist, item, NULL);
}

void
gtk_icon_list_unselect_all(GtkIconList *iconlist)
{
  unselect_all(iconlist);
}

void
gtk_icon_list_set_active_icon(GtkIconList *iconlist, GtkIconListItem *icon)
{
  if(!icon){
    deactivate_entry(iconlist);
    unselect_all(iconlist);
    return;
  }

  if(icon->entry){
    icon->state = GTK_STATE_SELECTED;
    entry_in(icon->entry, NULL, iconlist);
    gtk_widget_grab_focus(icon->entry);
  }
}


gboolean
gtk_icon_list_is_editable       (GtkIconList *iconlist)
{
  return (iconlist->is_editable);
}

guint
gtk_icon_list_get_row_spacing       (GtkIconList *iconlist)
{
  return(iconlist->row_spacing);
}

guint
gtk_icon_list_get_col_spacing       (GtkIconList *iconlist)
{
  return(iconlist->col_spacing);
}

guint
gtk_icon_list_get_text_space       (GtkIconList *iconlist)
{
  return(iconlist->text_space);
}

guint
gtk_icon_list_get_icon_border       (GtkIconList *iconlist)
{
  return(iconlist->icon_border);
}

guint
gtk_icon_list_get_icon_width       (GtkIconList *iconlist)
{
  return(iconlist->icon_width);
}

