/* Copyright (C)
 * 2015 - John Melton, G0ORX/N6LYT
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include <gtk/gtk.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#endif

#include "new_menu.h"
#include "andromeda_menu.h"

int andromeda_serial_enable = 0;
char andromeda_serial_port[64] = "/dev/ttyACM0";
#ifdef _WIN32
int andromeda_serial_baud_rate = 13; // B9600;
#else
int andromeda_serial_baud_rate = B9600;
#endif

gboolean andromeda_debug = FALSE;

static GtkWidget *parent_window = NULL;
static GtkWidget *menu_b = NULL;
static GtkWidget *dialog = NULL;
static GtkWidget *andromeda_serial_port_entry;

static void cleanup()
{
  if (dialog != NULL)
  {
    gtk_widget_destroy(dialog);
    dialog = NULL;
    sub_menu = NULL;
  }
}

static gboolean close_cb(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  cleanup();
  return TRUE;
}

static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  cleanup();
  return FALSE;
}

static void serial_value_changed_cb(GtkWidget *widget, gpointer data)
{
  sprintf(andromeda_serial_port, "%s", gtk_entry_get_text(GTK_ENTRY(widget)));
  fprintf(stderr, "ANDROMEDA_MENU: New Serial port=%s\n", andromeda_serial_port);
}

static void andromeda_debug_cb(GtkWidget *widget, gpointer data)
{
  andromeda_debug = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  g_print("---------- ANDROMEDA DEBUG %s ----------\n", andromeda_debug ? "ON" : "OFF");
}

static void serial_enable_cb(GtkWidget *widget, gpointer data)
{
  strcpy(andromeda_serial_port, gtk_entry_get_text(GTK_ENTRY(andromeda_serial_port_entry)));
  fprintf(stderr, "ANDROMEDA_MENU: New Serial port=%s\n", andromeda_serial_port);
  andromeda_serial_enable = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  if (andromeda_serial_enable)
  {
    if (launch_serial() == 0)
    {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), FALSE);
    }
  }
  else
  {
    disable_serial();
  }
}

// Set Baud Rate
static void baud_rate_cb(GtkWidget *widget, gpointer UNUSED(data))
{
  char *brSelect = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(widget));
  switch (atoi(brSelect))
  {
  case 4800:
    andromeda_serial_baud_rate = B4800;
    break;
  case 9600:
    andromeda_serial_baud_rate = B9600;
    break;
  case 19200:
    andromeda_serial_baud_rate = B19200;
    break;
  case 38400:
    andromeda_serial_baud_rate = B38400;
    break;
  default:
    andromeda_serial_baud_rate = B9600;
  }
  fprintf(stderr, "Andromeda_MENU: Baud rate changed: %s\n", brSelect);
}

void create_combobox_br(GtkWidget *parent)
{
  GtkWidget *baud_rate_combobox = gtk_combo_box_text_new();
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(baud_rate_combobox), NULL, "4800");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(baud_rate_combobox), NULL, "9600");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(baud_rate_combobox), NULL, "19200");
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(baud_rate_combobox), NULL, "38400");

  switch (andromeda_serial_baud_rate)
  {
  case B4800:
    gtk_combo_box_set_active(GTK_COMBO_BOX(baud_rate_combobox), 0);
    break;
  case B9600:
    gtk_combo_box_set_active(GTK_COMBO_BOX(baud_rate_combobox), 1);
    break;
  case B19200:
    gtk_combo_box_set_active(GTK_COMBO_BOX(baud_rate_combobox), 2);
    break;
  case B38400:
    gtk_combo_box_set_active(GTK_COMBO_BOX(baud_rate_combobox), 3);
    break;

  default:
    gtk_combo_box_set_active(GTK_COMBO_BOX(baud_rate_combobox), 1);
    break;
  }

  g_signal_connect(baud_rate_combobox, "changed", G_CALLBACK(baud_rate_cb), NULL);
  gtk_widget_show(baud_rate_combobox);
  gtk_grid_attach(GTK_GRID(parent), baud_rate_combobox, 2, 3, 4, 1);
}

void andromeda_menu(GtkWidget *parent)
{
  parent_window = parent;

  dialog = gtk_dialog_new();
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent_window));
  gtk_window_set_title(GTK_WINDOW(dialog), "piHPSDR - Andromeda");
  g_signal_connect(dialog, "delete_event", G_CALLBACK(delete_event), NULL);

  GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

  GtkWidget *close_b = gtk_button_new_with_label("Close");
  g_signal_connect(close_b, "pressed", G_CALLBACK(close_cb), NULL);
  gtk_grid_attach(GTK_GRID(grid), close_b, 0, 0, 1, 1);

  GtkWidget *serial_enable_b = gtk_check_button_new_with_label("Andromeda Enable");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(serial_enable_b), andromeda_serial_enable);
  gtk_widget_show(serial_enable_b);
  gtk_grid_attach(GTK_GRID(grid), serial_enable_b, 1, 1, 1, 1);
  g_signal_connect(serial_enable_b, "toggled", G_CALLBACK(serial_enable_cb), NULL);

  GtkWidget *serial_text_label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(serial_text_label), "<b>Serial Port:</b>");
  gtk_grid_attach(GTK_GRID(grid), serial_text_label, 1, 2, 1, 1);

  GtkWidget *baud_rate_label = gtk_label_new(NULL);
  gtk_label_set_markup(GTK_LABEL(baud_rate_label), "<b>Baud Rate:</b>");
  gtk_widget_show(baud_rate_label);
  gtk_grid_attach(GTK_GRID(grid), baud_rate_label, 1, 3, 1, 1);

  andromeda_serial_port_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(andromeda_serial_port_entry), andromeda_serial_port);
  g_signal_connect(andromeda_serial_port_entry, "activate", G_CALLBACK(serial_value_changed_cb), NULL);
  gtk_widget_show(andromeda_serial_port_entry);
  gtk_grid_attach(GTK_GRID(grid), andromeda_serial_port_entry, 2, 2, 4, 1);

  create_combobox_br(grid);

  GtkWidget *andromeda_debug_b = gtk_check_button_new_with_label("Debug Andromeda");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(andromeda_debug_b), andromeda_debug);
  gtk_widget_show(andromeda_debug_b);
  gtk_grid_attach(GTK_GRID(grid), andromeda_debug_b, 1, 0, 1, 1);
  g_signal_connect(andromeda_debug_b, "toggled", G_CALLBACK(andromeda_debug_cb), NULL);

  gtk_container_add(GTK_CONTAINER(content), grid);
  sub_menu = dialog;
  gtk_widget_show_all(dialog);
}
