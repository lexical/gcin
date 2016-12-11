#include "gcin.h"
#include "pho.h"
#include "gtab.h"
#include "win-sym.h"
#include "eggtrayicon.h"
#include <string.h>
#if UNIX
#include <signal.h>
#else
#include <process.h>
#endif
#include "gst.h"
#include "pho-kbm-name.h"

gboolean tsin_pho_mode();
void toggle_half_full_char_sub();

extern int tsin_half_full, gb_output;
extern int win32_tray_disabled;
GtkStatusIcon *icon_main=NULL, *icon_state=NULL;

void get_icon_path(char *iconame, char fname[]);

void cb_trad_sim_toggle();

#if WIN32
void cb_sim2trad(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  win32exec("sim2trad.exe");
}
void cb_trad2sim(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  win32exec("trad2sim.exe");
}
#else
void cb_sim2trad(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  system(GCIN_BIN_DIR"/sim2trad &");
}

void cb_trad2sim(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  system(GCIN_BIN_DIR"/trad2sim &");
}
#endif


void cb_tog_phospeak(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  phonetic_speak= gtk_check_menu_item_get_active(checkmenuitem);
}


void close_all_clients();
void do_exit();

void restart_gcin(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  do_exit();
}

void gcb_main();
void cb_tog_gcb(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
#if USE_GCB
  gcb_enabled = gtk_check_menu_item_get_active(checkmenuitem);
//  dbg("gcb_enabled %d\n", gcb_enabled);
  gcb_main();
#endif
}


void kbm_toggle(), exec_gcin_setup(), restart_gcin(), cb_trad2sim(), cb_sim2trad();

void cb_trad2sim(GtkCheckMenuItem *checkmenuitem, gpointer dat);

void cb_trad_sim_toggle_(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  cb_trad_sim_toggle();
//  dbg("checkmenuitem %x\n", checkmenuitem);
}

void toggle_stat_win();
void cb_stat_toggle_(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  toggle_stat_win();
}


void exec_gcin_setup_(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  exec_gcin_setup();
}

void kbm_open_close(gboolean b_show);
void kbm_toggle_(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  kbm_open_close(gtk_check_menu_item_get_active(checkmenuitem));
}

void create_about_window();

void cb_about_window(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  create_about_window();
}

gint inmd_switch_popup_handler (GtkWidget *widget, GdkEvent *event);
extern gboolean win_kbm_inited;

#include "mitem.h"
extern int win_kbm_on;

static MITEM mitems_main[] = {
  {"關於gcin/常見問題", GTK_STOCK_ABOUT, cb_about_window},
  {"設定/工具", GTK_STOCK_PREFERENCES, exec_gcin_setup_},
#if USE_GCB
  {"gcb(剪貼區暫存)", NULL, cb_tog_gcb, &gcb_enabled},
#endif
  {"重新執行gcin", GTK_STOCK_QUIT, restart_gcin},
  {"念出發音", NULL, cb_tog_phospeak, &phonetic_speak},
  {"小鍵盤", NULL, kbm_toggle_, &win_kbm_on},
#if UNIX && 0
  {"選擇輸入法", GTK_STOCK_INDEX, cb_inmd_menu, NULL},
#endif
  {NULL}
};


void set_output_buffer_bak_to_clipboard();
void set_output_buffer_bak_to_clipboard();
void cb_set_output_buffer_bak_to_clipboard(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  set_output_buffer_bak_to_clipboard();
}

void load_setttings(), load_tab_pho_file();;
void update_win_kbm();
void update_win_kbm_inited();
extern gboolean win_kbm_inited, stat_enabled;

void fast_phonetic_kbd_switch()
{
  char bak[128], cur[128];
  get_gcin_conf_fstr(PHONETIC_KEYBOARD, cur, "");
  get_gcin_conf_fstr(PHONETIC_KEYBOARD_BAK, bak, "");

  save_gcin_conf_str(PHONETIC_KEYBOARD, bak);
  save_gcin_conf_str(PHONETIC_KEYBOARD_BAK, cur);
  load_setttings();
  load_tab_pho_file();
  update_win_kbm_inited();
}

void cb_fast_phonetic_kbd_switch(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
	fast_phonetic_kbd_switch();
}

void cb_half_full_char(GtkCheckMenuItem *checkmenuitem, gpointer dat)
{
  toggle_half_full_char_sub();
}


static MITEM mitems_state[] = {
  {NULL, NULL, cb_fast_phonetic_kbd_switch},
  {"正→簡體", NULL, cb_trad2sim},
  {"簡→正體", NULL, cb_sim2trad},
  {"简体输出", NULL, cb_trad_sim_toggle_, &gb_output},
  {"打字速度", NULL, cb_stat_toggle_, &stat_enabled},
  {"全半形切換", NULL, cb_half_full_char, NULL},
  {"送字到剪貼區", NULL, cb_set_output_buffer_bak_to_clipboard},
  {NULL}
};


static GtkWidget *tray_menu=NULL, *tray_menu_state=NULL;


void toggle_im_enabled();
GtkWidget *create_tray_menu(MITEM *mitems)
{
  GtkWidget *menu = gtk_menu_new ();

  int i;
  for(i=0; mitems[i].cb; i++) {
    GtkWidget *item;

    if (!mitems[i].name)
      continue;

    if (mitems[i].stock_id) {
      item = gtk_image_menu_item_new_with_label (mitems[i].name);
      gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(item), gtk_image_new_from_stock(mitems[i].stock_id, GTK_ICON_SIZE_MENU));
    }
    else
    if (mitems[i].check_dat) {
      item = gtk_check_menu_item_new_with_label (mitems[i].name);
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), *mitems[i].check_dat);
    } else
      item = gtk_menu_item_new_with_label (mitems[i].name);

    mitems[i].handler = g_signal_connect (G_OBJECT (item), "activate",
                      G_CALLBACK (mitems[i].cb), NULL);

    gtk_widget_show(item);
    mitems[i].item = item;

    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  }

  return menu;
}

void update_item_active(MITEM *mitems)
{
  int i;
  for(i=0; mitems[i].name; i++)
    if (mitems[i].check_dat) {
      GtkWidget *item = mitems[i].item;
      if (!item)
        continue;

      g_signal_handler_block(item, mitems[i].handler);
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), *mitems[i].check_dat);
      g_signal_handler_unblock(item, mitems[i].handler);
    }
}

void update_item_active_unix();

void update_item_active_all()
{
  if (gcin_win32_icon) {
    update_item_active(mitems_main);
    update_item_active(mitems_state);
  }
#if UNIX
  else
    update_item_active_unix();
#endif
}


void inmd_popup_tray();

static void cb_activate(GtkStatusIcon *status_icon, gpointer user_data)
{
  inmd_popup_tray();
}

static void cb_popup(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
//  dbg("cb_popup\n");
  switch (button) {
#if UNIX
    case 1:
      toggle_im_enabled();

      break;
    case 2:
      kbm_toggle();
      break;
#endif
    case 3:
//      dbg("tray_menu %x\n", tray_menu);
      if (!tray_menu)
        tray_menu = create_tray_menu(mitems_main);
      gtk_menu_popup(GTK_MENU(tray_menu), NULL, NULL, NULL, NULL, button, activate_time);
      break;
  }
}


static void cb_activate_state(GtkStatusIcon *status_icon, gpointer user_data)
{
//  dbg("cb_activate\n");
  if (gcin_tray_hf_win_kbm) {
    kbm_toggle();
    update_item_active_all();
  } else
    toggle_half_full_char_sub();
}


static void cb_popup_state(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data)
{
  switch (button) {

    case 1:
#if UNIX
      toggle_im_enabled();
      break;
    case 2:
#endif
      kbm_toggle();
      break;
    case 3:
    {
      char bak[512], cur[512];
      get_gcin_conf_fstr(PHONETIC_KEYBOARD, cur, "");
      get_gcin_conf_fstr(PHONETIC_KEYBOARD_BAK, bak, "");
      if (bak[0] && strcmp(bak, cur)) {
        char kbm[512];

        strcpy(kbm, bak);
        char *p=strchr(kbm, ' ');

        if (p) {
          *(p++)=0;
          int i;
          for(i=0;kbm_sel[i].name;i++)
            if (!strcmp(kbm_sel[i].kbm, kbm)) {
              break;
            }

          if (kbm_sel[i].kbm) {
            char tt[128];
            if (mitems_state[0].name)
              free(mitems_state[0].name);

            sprintf(tt, "注音換 %s %s", kbm_sel[i].name, p);
            mitems_state[0].name = strdup(tt);
          }
        }

//        dbg("hhhhhhhhhhhh %x\n", tray_menu_state);
        if (tray_menu_state) {
          gtk_widget_destroy(tray_menu_state);
          tray_menu_state = NULL;
        }
      }

      if (!tray_menu_state)
        tray_menu_state = create_tray_menu(mitems_state);

      gtk_menu_popup(GTK_MENU(tray_menu_state), NULL, NULL, NULL, NULL, button, activate_time);
      break;
    }
  }

//  dbg("zzzzzzzzzzzzz\n");
}


#define GCIN_TRAY_PNG "gcin_tray.png"

void disp_win_screen_status(char *in_method, char *half_status);
extern gboolean capslock_on;

void load_tray_icon_win32()
{
#if WIN32
  // when login, creating icon too early may cause block in gtk_status_icon_new_from_file
  if (win32_tray_disabled /* || !gcin_status_tray */)
    return;
#endif

//  dbg("load_tray_icon_win32\n");
#if UNIX
  char *tip;
  tip="";
#else
  wchar_t *tip;
  tip=L"";
#endif

  char *iconame="en-tsin.png";
  char tt[32];
  strcpy(tt, iconame);



  if (!current_CS || current_CS->im_state == GCIN_STATE_DISABLED||current_CS->im_state == GCIN_STATE_ENG_FULL) {
    iconame=capslock_on?"en-gcin-A.png":GCIN_TRAY_PNG;
  } else {
    iconame=inmd[current_CS->in_method].icon;
  }

//  dbg("caps %d %s\n", capslock_on, iconame);

//  dbg("tsin_pho_mode() %d\n", tsin_pho_mode());

  gboolean is_tsin = current_method_type()==method_type_TSIN;

  if (current_CS && current_CS->im_state == GCIN_STATE_CHINESE && !tsin_pho_mode()) {
    if ((is_tsin || current_method_type()==method_type_MODULE)) {
      strcpy(tt, "en-");
      strcat(tt, iconame);
      if (capslock_on && is_tsin)
        strcpy(tt, "en-tsin-A.png");
    } else {
      if (current_method_type()==method_type_GTAB) {
        strcpy(tt, capslock_on?"en-gtab-A.png":"en-gtab.png");
       }
    }

    iconame = tt;
  }

//  dbg("iconame %s\n", iconame);
  char fname[128];
  fname[0]=0;
  if (iconame)
    get_icon_path(iconame, fname);


  char *icon_st=NULL;
  char fname_state[128];

//  dbg("%d %d\n",current_CS->im_state,current_CS->b_half_full_char);

  if (current_CS && (current_CS->im_state == GCIN_STATE_ENG_FULL ||
      current_CS->im_state != GCIN_STATE_DISABLED && current_CS->b_half_full_char ||
      current_method_type()==method_type_TSIN && tss.tsin_half_full)) {
      if (gb_output) {
        icon_st="full-simp.png";
        tip = _L("全形/簡體輸出");
      }
      else {
        icon_st="full-trad.png";
        tip = _L("全形/正體輸出");
      }
  } else {
    if (gb_output) {
      icon_st="half-simp.png";
      tip= _L("半形/簡體輸出");
    } else {
      icon_st="half-trad.png";
      tip = _L("半形/正體輸出");
    }
  }

  get_icon_path(icon_st, fname_state);
//  dbg("wwwwwwww %s\n", fname_state);

  if (gcin_status_win)
    disp_win_screen_status(fname, fname_state);


  if (!gcin_status_tray)
    return;


#if UNIX
  if (gcin_win32_icon==GCIN_TRAY_UNIX)
    return;
#endif

  if (icon_main) {
//    dbg("set %s %s\n", fname, fname_state);
    gtk_status_icon_set_from_file(icon_main, fname);
    gtk_status_icon_set_from_file(icon_state, fname_state);
  }
  else {
//    dbg("gtk_status_icon_new_from_file a\n");
    icon_main = gtk_status_icon_new_from_file(fname);
    g_signal_connect(G_OBJECT(icon_main),"activate", G_CALLBACK (cb_activate), NULL);
    g_signal_connect(G_OBJECT(icon_main),"popup-menu", G_CALLBACK (cb_popup), NULL);

//	dbg("gtk_status_icon_new_from_file %s b\n", fname_state);
    icon_state = gtk_status_icon_new_from_file(fname_state);
    g_signal_connect(G_OBJECT(icon_state),"activate", G_CALLBACK (cb_activate_state), NULL);
    g_signal_connect(G_OBJECT(icon_state),"popup-menu", G_CALLBACK (cb_popup_state), NULL);

//	dbg("icon %s %s\n", fname, fname_state);
  }

#if GTK_CHECK_VERSION(2,16,0)
  if (icon_state)
    gtk_status_icon_set_tooltip_text(icon_state, _(tip));
#endif

  if (icon_main) {
    char tt[64];
    if (current_CS && inmd[current_CS->in_method].cname && inmd[current_CS->in_method].cname[0]) {
//      dbg("cname %s\n", inmd[current_CS->in_method].cname);
      strcpy(tt, inmd[current_CS->in_method].cname);
    }

    if (!iconame || !strcmp(iconame, GCIN_TRAY_PNG) || !tsin_pho_mode())
      strcpy(tt, "English");
#if GTK_CHECK_VERSION(2,16,0)
    gtk_status_icon_set_tooltip_text(icon_main, tt);
#endif
  }

  return;
}

void init_tray_win32()
{
  load_tray_icon_win32();
}

void destroy_tray_win32()
{
  if (tray_menu!=NULL) {
    gtk_widget_destroy(tray_menu);
    tray_menu = NULL;
  }

  if (tray_menu_state!=NULL) {
    gtk_widget_destroy(tray_menu_state);
    tray_menu_state = NULL;
  }


  g_object_unref(icon_main); icon_main = NULL;
  g_object_unref(icon_state); icon_state = NULL;
}
