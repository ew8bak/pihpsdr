#include <gtk/gtk.h>
#include "css.h"

char *css=
"  @define-color TOGGLE_ON rgb(100%,0%,0%);\n"
"  @define-color TOGGLE_OFF rgb(70%,70%,70%);\n"
"  #small_button {\n"
"    padding: 0;\n"
"    font-family: Sans;\n"
"    font-size: 15px;\n"
"    }\n"
"  #small_toggle_button {\n"
"    padding: 0;\n"
"    font-family: Sans;\n"
"    font-size: 15px;\n"
"    background-image: none;\n"
"    background-color: @TOGGLE_OFF;\n"
"    }\n"
"  #small_toggle_button:checked {\n"
"    padding: 0;\n"
"    font-family: Sans;\n"
"    font-size: 15px;\n"
"    background-image: none;\n"
"    background-color: @TOGGLE_ON;\n"
"    }\n"

;

void load_css() {
  GtkCssProvider *provider;
  GdkDisplay *display;
  GdkScreen *screen;

  g_print("%s\n",__FUNCTION__);
  provider = gtk_css_provider_new ();
  display = gdk_display_get_default ();
  screen = gdk_display_get_default_screen (display);
  gtk_style_context_add_provider_for_screen (screen,
                                             GTK_STYLE_PROVIDER(provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  gtk_css_provider_load_from_data(provider, css, -1, NULL);
  g_object_unref (provider);
}
