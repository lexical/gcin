#include "gcin.h"

#define MAX_KEPT_TIME (5 * 60 * (gint64)1000000)
static gint64 *ch_time;
static int ch_timeN, ch_timeN_a;
static GtkWidget *label_disp, *gwin_stat;
gboolean stat_enabled;
static int timeout_handle;

static int get_ch_count(int mini)
{
  gint64 t = current_time() -  mini * 60 * 1000000;

  int i, N;
  for(i=0,N=0; i < ch_timeN; i++)
    if (ch_time[i] >= t)
      N++;

 return ((double)N/mini + 0.5);
}


void disp_stat()
{
  char tt[512];

  sprintf(tt, _(_L("1,3,5分鐘\n%d,%d,%d/分")), get_ch_count(1), get_ch_count(3), get_ch_count(5));
  gtk_label_set_text(GTK_LABEL(label_disp), tt);
#if WIN32
  gtk_window_present(GTK_WINDOW(gwin_stat));
#endif
}

void add_ch_time()
{
  int i;
  gint64 tim = current_time();
  gint64 tim_exp = tim - MAX_KEPT_TIME;

  for(i=0;i<ch_timeN;i++)
    if (ch_time[i] > tim_exp)
      break;

  if (i) {
    int leftN = ch_timeN - i;
    memmove(ch_time, ch_time+i, sizeof(gint64) * leftN);
    ch_timeN = leftN;
  }

  if (ch_timeN_a <= ch_timeN+1) {
    ch_timeN_a =ch_timeN+1;
    ch_time = trealloc(ch_time, gint64, ch_timeN_a);
  }

  ch_time[ch_timeN++]=tim;
}

void add_ch_time_str(char *s)
{
  int len= strlen(s);
  int i=0;
  while (i< len) {
    if (!(s[i] & 0x80)) {
      i++;
      continue;
    }
    i+=utf8_sz(s+i);
    add_ch_time();
  }

  if (stat_enabled)
    disp_stat();
}

void destory_win_stat()
{
  if (!gwin_stat)
    return;
  gtk_widget_destroy(gwin_stat);
  gwin_stat = NULL;
  g_source_remove(timeout_handle);
}


gboolean timeout_update_stat(gpointer data)
{
  disp_stat();
  return TRUE;
}


void create_stat_win()
{
  gwin_stat = create_no_focus_win ();

  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (gwin_stat), vbox);


  label_disp = gtk_label_new(NULL);

  gtk_box_pack_start (GTK_BOX (vbox), label_disp, TRUE, TRUE, 0);

  gtk_widget_show_all(gwin_stat);
  timeout_handle = g_timeout_add(3000, timeout_update_stat, NULL);
}



void toggle_stat_win()
{
  stat_enabled ^= 1;
  if (stat_enabled) {
    create_stat_win();
    disp_stat();
  } else
    destory_win_stat();
}
