 #include <gtk/gtk.h>
 #include "rkusb.h"

 static rk_usb_device *di = NULL;

 void append_text(GtkTextBuffer *data, char *buf) {
  GtkTextIter iter;
  GtkTextMark *cursor;

  /* Get the mark at cursor. */
  cursor = gtk_text_buffer_get_mark (data, "insert");
  /* Get the iter at cursor. */
  gtk_text_buffer_get_iter_at_mark (data, &iter, cursor);
  gtk_text_buffer_insert (data, &iter, buf, -1);

 }

void reset_device(GtkWidget *widget, GtkTextBuffer *data) {
    append_text(data, "Reset device\r\n");
    send_reset(di, 3);
    recv_res(di);
}

 void scan_device(GtkWidget *widget, GtkTextBuffer *data) {

    char *str = malloc(256);
    if (di) {
        disconnect(di);
    }
    di = allocate_rk_usb_device();
    if ( !(di = connect_usb_device(di)) )  {
        append_text(data, "Unable to connect device\r\n");
        return;
    }

    snprintf(str, 128, "Detected soc: %s with mode: %s\r\n", di->soc, di->mode == RKFT_USB_MODE_MASKROM ? "MASKROM" : "LOADER");
    append_text(data, str);

    free(str);
}

 void app_activate (GApplication *app, gpointer *user_data) {
    GtkWidget *win, *grid, *tv, *btn_scan, *btn_reset;
    GtkTextBuffer *tb;

    win = gtk_application_window_new (GTK_APPLICATION (app));
    gtk_window_set_title (GTK_WINDOW (win), "GRKFlashTool");
    gtk_window_set_default_size (GTK_WINDOW (win), 600, 300);

    /* Here we construct the container that is going pack our buttons */
    grid = gtk_grid_new();

    gtk_widget_set_hexpand(grid, true);
    gtk_widget_set_vexpand(grid, true);

    tv = gtk_text_view_new ();
    tb = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tv));
    gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (tv), GTK_WRAP_WORD_CHAR);
    gtk_widget_set_hexpand(tv, true);
    gtk_widget_set_vexpand(tv, true);
    gtk_grid_attach (GTK_GRID (grid), tv, 0, 1, 8, 1);

    btn_scan = gtk_button_new_with_label ("Scan Device");
    gtk_grid_attach (GTK_GRID (grid), btn_scan, 0, 0, 1, 1);

    btn_reset = gtk_button_new_with_label ("Reset Device");
    gtk_grid_attach (GTK_GRID (grid), btn_reset, 1, 0, 1, 1);

    g_signal_connect(G_OBJECT(btn_reset), "clicked", 
      G_CALLBACK(reset_device), tb);
    
    g_signal_connect(G_OBJECT(btn_scan), "clicked", 
      G_CALLBACK(scan_device), tb);

    gtk_window_set_child (GTK_WINDOW (win), grid);

    gtk_widget_show (win);
  }
 
int main (int argc, char **argv) {
    GtkApplication *app;
    int stat;

    app = gtk_application_new ("com.github.ilmich.rkflashtool", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (app_activate), NULL);
    stat =g_application_run (G_APPLICATION (app), argc, argv);
    if (di != NULL) {
        disconnect(di);
    }
    g_object_unref (app);
    return stat;
 }

