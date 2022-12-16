 #include <gtk/gtk.h>

 void app_activate (GApplication *app, gpointer *user_data) {
    GtkWidget *win;
    GtkWidget *tv;
    GtkTextBuffer *tb;
    gchar *text;

    win = gtk_application_window_new (GTK_APPLICATION (app));
    gtk_window_set_title (GTK_WINDOW (win), "GRKFlashTool");
    gtk_window_set_default_size (GTK_WINDOW (win), 600, 300);

   tv = gtk_text_view_new ();
   tb = gtk_text_view_get_buffer (GTK_TEXT_VIEW (tv));
   gtk_text_buffer_set_text (tb, text, -1);
   gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (tv), GTK_WRAP_WORD_CHAR);
 
   gtk_window_set_child (GTK_WINDOW (win), tv);

    gtk_widget_show (win);
  }
 
int main (int argc, char **argv) {
    GtkApplication *app;
    int stat;

    app = gtk_application_new ("com.github.ilmich.rkflashtool", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (app_activate), NULL);
    stat =g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    return stat;
 }

