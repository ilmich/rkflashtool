#include <gtk/gtk.h>
#include "rkusb.h"
#include "rkbin.h"

static GtkWidget *win;

void append_text(GtkTextBuffer *data, char *buf) {
  GtkTextIter iter;
  GtkTextMark *cursor;

  /* Get the mark at cursor. */
  cursor = gtk_text_buffer_get_mark (data, "insert");
  /* Get the iter at cursor. */
  gtk_text_buffer_get_iter_at_mark (data, &iter, cursor);
  gtk_text_buffer_insert (data, &iter, buf, -1);
  gtk_widget_queue_draw(win);

  while (g_main_context_pending(NULL)) g_main_context_iteration(NULL, FALSE);
 }

 gpointer* erase_thread ( gpointer user_data ) {

 };

void erase_device(GtkWidget *widget, GtkTextBuffer *data) {
    rkusb_device *di = NULL;
    char *str = malloc(256);
    int size = 0xf424, offset=0, perc = 0;
    GThread* thread;

    while (g_main_context_pending(NULL)) g_main_context_iteration(NULL, FALSE);

    if ( !(di = rkusb_connect_device(di)) )  {
        append_text(data, "Unable to connect device\r\n");
        return;
    }

    if (di->mode != RKFT_USB_MODE_MASKROM && di->mode != RKFT_USB_MODE_LOADER) {
        info("Something is wrong\r\n");
        append_text(data, "Something is wrong\r\n");
        return ;
    } else {
        int completed = 0;
        //thread = g_thread_new("erase_thread",erase_thread
        memset(di->buf, 0xff, RKFT_BLOCKSIZE);
        while (size > 0) {
            
            if (completed % (640 * 10) == 0) {
                snprintf(str, 256, "Erase flash in progress %d\r\n", perc);
                append_text(data, str);
                perc += 10;
            }
            infocr("erasing flash memory at offset 0x%08x", offset);
            rkusb_send_cmd(di, RKFT_CMD_WRITELBA, offset, RKFT_OFF_INCR);
            rkusb_send_buf(di,RKFT_BLOCKSIZE);
            rkusb_recv_res(di);

            offset += RKFT_OFF_INCR;
            size   -= RKFT_OFF_INCR;
            completed += RKFT_OFF_INCR;
        }
        append_text(data, "Erase flash completed\r\n");
    }
    free(str);
    rkusb_disconnect(di);
}

void reset_device(GtkWidget *widget, GtkTextBuffer *data) {
    /*append_text(data, "Reset device\r\n");
    send_reset(di, 3);
    recv_res(di);*/
}

void scan_device(GtkWidget *widget, GtkTextBuffer *data) {
/*
    char *str = malloc(256);
    if (di) {
        disconnect(di);
    }
    di = allocate_rkusb_device();
    if ( !(di = connect_usb_device(di)) )  {
        append_text(data, "Unable to connect device\r\n");
        return;
    }

    snprintf(str, 128, "Detected soc: %s with mode: %s\r\n", di->soc, di->mode == RKFT_USB_MODE_MASKROM ? "MASKROM" : "LOADER");
    append_text(data, str);
    free(str);*/
}

void app_activate (GApplication *app, gpointer *user_data) {
    GtkWidget *grid, *tv, *box_tools;
    GtkWidget *btn_erase;
    //GtkWidget *lbl_soc, *lbl_mode;
    GtkWidget *tools_frame;
    GtkWidget *progress;
    GtkAdjustment *adj;
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
    gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
    gtk_widget_set_hexpand(tv, true);
    gtk_widget_set_vexpand(tv, true);
    gtk_grid_attach (GTK_GRID (grid), tv, 0, 2, 8, 1);

    // frame
    tools_frame = gtk_frame_new("Tools");
    gtk_grid_attach (GTK_GRID (grid), tools_frame, 0, 0, 8, 1);
    box_tools = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_frame_set_child(GTK_FRAME(tools_frame), box_tools);
/*
    flash_frame = gtk_frame_new("Flash raw image");
    gtk_grid_attach (GTK_GRID (grid), flash_frame, 0, 1, 8, 1);
*/
    btn_erase = gtk_button_new_with_label ("Erase Flash");
    g_signal_connect(G_OBJECT(btn_erase), "clicked", 
      G_CALLBACK(erase_device), tb);
    gtk_box_append(GTK_BOX(box_tools), btn_erase);

    gtk_window_set_child (GTK_WINDOW (win), grid);
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
