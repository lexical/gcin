#include "gcin.h"
#include "gtab.h"


static GtkWidget *check_button_gtab_dup_select_bell,
                 *opt_gtab_press_full_auto_send,
                 *opt_gtab_pre_select,
                 *opt_gtab_disp_partial_match,
                 *check_button_gtab_disp_key_codes,
                 *check_button_gtab_disp_im_name,
                 *check_button_gtab_invalid_key_in,
                 *check_button_gtab_shift_phrase_key,
                 *check_button_gtab_hide_row2,
                 *check_button_gtab_in_row1,
                 *opt_gtab_vertical_select,
                 *opt_gtab_unique_auto_send,
                 *check_button_gtab_que_wild_card,
                 *check_button_gtab_phrase_pre_select;

extern GtkWidget *check_button_gcin_capslock_lower, *opt_tsin_space;
extern gboolean button_order;

struct {
  char *str;
  int num;
} spc_opts[] = {
  {"由 .gtab 指定", GTAB_space_auto_first_none},
  {"按空白立即送出第一字(嘸蝦米、大易)", GTAB_space_auto_first_any},
  {"按滿按空白送出第一字", GTAB_space_auto_first_full},
  {"按滿按空白不送出第一字(倉頡, 行列)", GTAB_space_auto_first_nofull},
  { NULL, 0},
};


struct {
  char *str;
  int num;
} auto_select_by_phrase_opts[] = {
  {"由.gtab指定", GTAB_OPTION_AUTO},
  {"全部開啟", GTAB_OPTION_YES},
  {"全部關閉", GTAB_OPTION_NO},
  { NULL, 0},
};

void save_tsin_eng_pho_key();
static GtkWidget *gcin_gtab_conf_window;
static GtkWidget *opt_spc_opts, *opt_auto_select_by_phrase;

void save_menu_val(char *config, GtkWidget *opt)
{
  int idx = gtk_combo_box_get_active (GTK_COMBO_BOX (opt));
  save_gcin_conf_int(config, auto_select_by_phrase_opts[idx].num);
}

void save_tsin_space_opt();

static gboolean cb_gtab_conf_ok( GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer   data )
{
  save_tsin_eng_pho_key();
  save_tsin_space_opt();
  save_gcin_conf_int(GTAB_DUP_SELECT_BELL,
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_gtab_dup_select_bell)));

  save_menu_val(GTAB_PRE_SELECT, opt_gtab_pre_select);

  save_menu_val(GTAB_DISP_PARTIAL_MATCH, opt_gtab_disp_partial_match);

  save_gcin_conf_int(GTAB_DISP_KEY_CODES,
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_gtab_disp_key_codes)));

  save_gcin_conf_int(GTAB_DISP_IM_NAME,
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_gtab_disp_im_name)));

  save_gcin_conf_int(GTAB_INVALID_KEY_IN,
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_gtab_invalid_key_in)));

  save_gcin_conf_int(GTAB_SHIFT_PHRASE_KEY,
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_gtab_shift_phrase_key)));

  save_gcin_conf_int(GTAB_HIDE_ROW2,
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_gtab_hide_row2)));

  save_gcin_conf_int(GTAB_IN_ROW1,
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_gtab_in_row1)));

  save_menu_val(GTAB_VERTICAL_SELECT, opt_gtab_vertical_select);

  save_gcin_conf_int(GTAB_QUE_WILD_CARD,
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_gtab_que_wild_card)));

  save_gcin_conf_int(GCIN_CAPSLOCK_LOWER,
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_gcin_capslock_lower)));

  save_gcin_conf_int(GTAB_PHRASE_PRE_SELECT,
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_gtab_phrase_pre_select)));

  int idx = gtk_combo_box_get_active (GTK_COMBO_BOX (opt_spc_opts));
  save_gcin_conf_int(GTAB_SPACE_AUTO_FIRST, spc_opts[idx].num);

  save_menu_val(GTAB_AUTO_SELECT_BY_PHRASE, opt_auto_select_by_phrase);

  save_menu_val(GTAB_PRESS_FULL_AUTO_SEND, opt_gtab_press_full_auto_send);

  save_menu_val(GTAB_UNIQUE_AUTO_SEND, opt_gtab_unique_auto_send);

  send_gcin_message(
#if UNIX
	  GDK_DISPLAY(),
#endif
	  CHANGE_FONT_SIZE);
  gtk_widget_destroy(gcin_gtab_conf_window); gcin_gtab_conf_window = NULL;

  return TRUE;
}


static gboolean close_gtab_conf_window( GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer   data )
{
  gtk_widget_destroy(gcin_gtab_conf_window); gcin_gtab_conf_window = NULL;
  return TRUE;
}

extern char utf8_edit[];
static gboolean cb_gtab_edit_append( GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer   data )
{
  load_gtab_list(FALSE);
  char *fname = inmd[default_input_method].filename;
  if (!fname)
    return TRUE;

  char append_fname[128];
#if UNIX
  sprintf(append_fname, "~/.gcin/%s.append", fname);

  char prepare[128];
  sprintf(prepare, GCIN_SCRIPT_DIR"/gtab.append_prepare %s", append_fname);
  system(prepare);

  char exec[128];

  sprintf(exec, "%s %s", utf8_edit, append_fname);
  dbg("exec %s\n", exec);
  system(exec);
#else
  strcat(strcpy(append_fname, fname), ".append");
  char user_fname[512];
  get_gcin_user_fname(append_fname, user_fname);
  if (GetFileAttributesA(user_fname) == INVALID_FILE_ATTRIBUTES)
    win32exec_script("gtab.append_prepare.bat", user_fname);
  else {
    win32exec_script("utf8-edit.bat", user_fname);
  }
#endif
  return TRUE;
}

static GtkWidget *create_spc_opts()
{
  GtkWidget *hbox = gtk_hbox_new (FALSE, 1);
  GtkWidget *label = gtk_label_new(_(_L("空白鍵選項")));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  opt_spc_opts = gtk_combo_box_new_text ();
#if !GTK_CHECK_VERSION(2,4,0)
  GtkWidget *menu_spc_opts = gtk_menu_new ();
#endif
  gtk_box_pack_start (GTK_BOX (hbox), opt_spc_opts, FALSE, FALSE, 0);

  int i, current_idx=0;

  for(i=0; spc_opts[i].str; i++) {
#if !GTK_CHECK_VERSION(2,4,0)
    GtkWidget *item = gtk_menu_item_new_with_label (_(spc_opts[i].str));
#endif

    if (spc_opts[i].num == gtab_space_auto_first)
      current_idx = i;

#if GTK_CHECK_VERSION(2,4,0)
    gtk_combo_box_append_text (GTK_COMBO_BOX_TEXT (opt_spc_opts), spc_opts[i].str);
#else
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_spc_opts), item);
#endif
  }

#if !GTK_CHECK_VERSION(2,4,0)
  gtk_option_menu_set_menu (GTK_OPTION_MENU (opt_spc_opts), menu_spc_opts);
#endif
  gtk_combo_box_set_active (GTK_COMBO_BOX (opt_spc_opts), current_idx);

  return hbox;
}

static GtkWidget *create_auto_select_by_phrase_opts(GtkWidget **out, int val)
{
  *out = gtk_combo_box_new_text ();
#if !GTK_CHECK_VERSION(2,4,0)
  GtkWidget *menu_auto_select_by_phrase = gtk_menu_new ();
#endif

  int i, current_idx=0;

  for(i=0; auto_select_by_phrase_opts[i].str; i++) {
#if !GTK_CHECK_VERSION(2,4,0)
    GtkWidget *item = gtk_menu_item_new_with_label (auto_select_by_phrase_opts[i].str);
#endif

    if (auto_select_by_phrase_opts[i].num == val)
      current_idx = i;

#if GTK_CHECK_VERSION(2,4,0)
    gtk_combo_box_append_text (GTK_COMBO_BOX_TEXT (*out), auto_select_by_phrase_opts[i].str);
#else
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_auto_select_by_phrase), item);
#endif
  }

#if !GTK_CHECK_VERSION(2,4,0)
  gtk_option_menu_set_menu (GTK_OPTION_MENU (*out), menu_auto_select_by_phrase);
#endif
  gtk_combo_box_set_active (GTK_COMBO_BOX (*out), current_idx);

  return *out;
}


int html_browser(char *fname);

static gboolean cb_gtab_help( GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer   data )
{
  html_browser("http://hyperrate.com/topic-files-dir/47/25947-m3_JEIC5FL/gtab_setting_help.html");
  return TRUE;
}



GtkWidget *create_en_pho_key_sel(char *s);
GtkWidget *create_tsin_space_opts();

void create_gtab_conf_window()
{
  if (gcin_gtab_conf_window) {
    gtk_window_present(GTK_WINDOW(gcin_gtab_conf_window));
    return;
  }

  load_setttings();

  gcin_gtab_conf_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(gcin_gtab_conf_window), GTK_WIN_POS_MOUSE);
  gtk_window_set_has_resize_grip(GTK_WINDOW(gcin_gtab_conf_window), FALSE);

  g_signal_connect (G_OBJECT (gcin_gtab_conf_window), "delete_event",
                    G_CALLBACK (close_gtab_conf_window),
                    NULL);

  gtk_window_set_title (GTK_WINDOW (gcin_gtab_conf_window), _(_L("倉頡/行列/嘸蝦米/大易設定")));
  gtk_container_set_border_width (GTK_CONTAINER (gcin_gtab_conf_window), 3);

  GtkWidget *vbox_top = gtk_vbox_new (FALSE, 10);
  gtk_container_add (GTK_CONTAINER (gcin_gtab_conf_window), vbox_top);

  GtkWidget *hbox_lr = gtk_hbox_new (FALSE, 10);
  gtk_box_pack_start (GTK_BOX (vbox_top), hbox_lr, FALSE, FALSE, 0);


  GtkWidget *frame_gtab_l = gtk_frame_new(_(_L("外觀")));
  gtk_container_set_border_width (GTK_CONTAINER (frame_gtab_l), 5);
  gtk_box_pack_start (GTK_BOX (hbox_lr), frame_gtab_l, FALSE, FALSE, 0);
  GtkWidget *vbox_gtab_l = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame_gtab_l), vbox_gtab_l);
  gtk_container_set_border_width (GTK_CONTAINER (vbox_gtab_l), 10);


  GtkWidget *frame_gtab_r = gtk_frame_new(_(_L("行為")));
  gtk_container_set_border_width (GTK_CONTAINER (frame_gtab_r), 5);
  gtk_box_pack_start (GTK_BOX (hbox_lr), frame_gtab_r, FALSE, FALSE, 0);
  GtkWidget *vbox_gtab_r = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (frame_gtab_r), vbox_gtab_r);
  gtk_container_set_border_width (GTK_CONTAINER (vbox_gtab_r), 10);

#define SPC 1

  GtkWidget *hbox_gtab_pre_select = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_l), hbox_gtab_pre_select, FALSE, FALSE, 0);
  GtkWidget *label_gtab_pre_select = gtk_label_new(_(_L("預覽/預選 字")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_pre_select), label_gtab_pre_select,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_gtab_pre_select), create_auto_select_by_phrase_opts(&opt_gtab_pre_select, gtab_pre_select),  FALSE, FALSE, 0);

  GtkWidget *hbox_gtab_disp_partial_match = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_l), hbox_gtab_disp_partial_match, FALSE, FALSE, 0);
  GtkWidget *label_gtab_gtab_disp_partial_match = gtk_label_new(_(_L("預選列中顯示部份符合的字")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_disp_partial_match), label_gtab_gtab_disp_partial_match,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_gtab_disp_partial_match), create_auto_select_by_phrase_opts(&opt_gtab_disp_partial_match, gtab_disp_partial_match), FALSE, FALSE, 0);

  GtkWidget *hbox_gtab_disp_key_codes = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_l), hbox_gtab_disp_key_codes, FALSE, FALSE, 0);
  GtkWidget *label_gtab_gtab_disp_key_codes = gtk_label_new(_(_L("顯示拆字")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_disp_key_codes), label_gtab_gtab_disp_key_codes,  FALSE, FALSE, 0);
  check_button_gtab_disp_key_codes = gtk_check_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox_gtab_disp_key_codes), check_button_gtab_disp_key_codes,  FALSE, FALSE, 0);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_gtab_disp_key_codes),
     gtab_disp_key_codes);


  GtkWidget *hbox_gtab_disp_im_name = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_l), hbox_gtab_disp_im_name, FALSE, FALSE, 0);
  GtkWidget *label_gtab_gtab_disp_im_name = gtk_label_new(_(_L("顯示輸入法名稱")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_disp_im_name), label_gtab_gtab_disp_im_name,  FALSE, FALSE, 0);
  check_button_gtab_disp_im_name = gtk_check_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox_gtab_disp_im_name), check_button_gtab_disp_im_name,  FALSE, FALSE, 0);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_gtab_disp_im_name),
     gtab_disp_im_name);

  GtkWidget *hbox_gtab_hide_row2 = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_l), hbox_gtab_hide_row2, FALSE, FALSE, 0);
  GtkWidget *label_gtab_hide_row2 = gtk_label_new(_(_L("隱藏第二列(輸入鍵…)")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_hide_row2), label_gtab_hide_row2,  FALSE, FALSE, 0);
  check_button_gtab_hide_row2 = gtk_check_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox_gtab_hide_row2), check_button_gtab_hide_row2,  FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_gtab_hide_row2),
     gtab_hide_row2);


  GtkWidget *hbox_gtab_in_row1 = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_l), hbox_gtab_in_row1, FALSE, FALSE, 0);
  GtkWidget *label_gtab_in_row1 = gtk_label_new(_(_L("輸入鍵顯示移至第一列")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_in_row1), label_gtab_in_row1,  FALSE, FALSE, 0);
  check_button_gtab_in_row1 = gtk_check_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox_gtab_in_row1), check_button_gtab_in_row1,  FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_gtab_in_row1),
     gtab_in_row1);

  GtkWidget *hbox_gtab_vertical_select = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_l), hbox_gtab_vertical_select, FALSE, FALSE, 0);
  GtkWidget *label_gtab_vertical_select = gtk_label_new(_(_L("垂直選擇")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_vertical_select), label_gtab_vertical_select,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_gtab_vertical_select), create_auto_select_by_phrase_opts(&opt_gtab_vertical_select, gtab_vertical_select),  FALSE, FALSE, 0);

  GtkWidget *hbox_gtab_press_full_auto_send = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_r), hbox_gtab_press_full_auto_send, FALSE, FALSE, 0);
  GtkWidget *label_gtab_gtab_press_full_auto_send = gtk_label_new(_(_L("按滿自動送字")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_press_full_auto_send), label_gtab_gtab_press_full_auto_send,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_gtab_press_full_auto_send), create_auto_select_by_phrase_opts(&opt_gtab_press_full_auto_send, gtab_press_full_auto_send),  FALSE, FALSE, 0);


  GtkWidget *hbox_gtab_auto_select_by_phrase = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_r), hbox_gtab_auto_select_by_phrase, FALSE, FALSE, 0);
  GtkWidget *label_gtab_auto_select = gtk_label_new(_(_L("由詞庫自動選擇字")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_auto_select_by_phrase), label_gtab_auto_select,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_gtab_auto_select_by_phrase), create_auto_select_by_phrase_opts(&opt_auto_select_by_phrase, gtab_auto_select_by_phrase),  FALSE, FALSE, 0);
  GtkWidget *label_gtab_phrase_pre_select = gtk_label_new(_(_L("使用預選詞")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_auto_select_by_phrase), label_gtab_phrase_pre_select,  FALSE, FALSE, 0);
  check_button_gtab_phrase_pre_select = gtk_check_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox_gtab_auto_select_by_phrase), check_button_gtab_phrase_pre_select,  FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_gtab_phrase_pre_select), gtab_phrase_pre_select);


  GtkWidget *hbox_gtab_dup_select_bell = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_r), hbox_gtab_dup_select_bell, FALSE, FALSE, 0);
  GtkWidget *label_gtab_sele = gtk_label_new(_(_L("重複字選擇鈴聲")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_dup_select_bell), label_gtab_sele,  FALSE, FALSE, 0);
  check_button_gtab_dup_select_bell = gtk_check_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox_gtab_dup_select_bell),check_button_gtab_dup_select_bell,  FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_gtab_dup_select_bell),
     gtab_dup_select_bell);

  gtk_box_pack_start (GTK_BOX (vbox_gtab_r), create_spc_opts(), FALSE, FALSE, 0);

  GtkWidget *hbox_gtab_invalid_key_in = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_r), hbox_gtab_invalid_key_in, FALSE, FALSE, 0);
  GtkWidget *label_gtab_gtab_invalid_key_in = gtk_label_new(_(_L("允許錯誤鍵進入(傳統)")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_invalid_key_in), label_gtab_gtab_invalid_key_in,  FALSE, FALSE, 0);
  check_button_gtab_invalid_key_in = gtk_check_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox_gtab_invalid_key_in), check_button_gtab_invalid_key_in,  FALSE, FALSE, 0);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_gtab_invalid_key_in),
     gtab_invalid_key_in);


  GtkWidget *hbox_gtab_shift_phrase_key = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_r), hbox_gtab_shift_phrase_key, FALSE, FALSE, 0);
  GtkWidget *label_gtab_gtab_shift_phrase_key = gtk_label_new(_(_L("Shift 用來輸入片語(Alt-Shift)")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_shift_phrase_key), label_gtab_gtab_shift_phrase_key,  FALSE, FALSE, 0);
  check_button_gtab_shift_phrase_key = gtk_check_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox_gtab_shift_phrase_key), check_button_gtab_shift_phrase_key,  FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_gtab_shift_phrase_key),
     gtab_shift_phrase_key);

  gtk_box_pack_start (GTK_BOX (vbox_gtab_r), create_en_pho_key_sel(_(_L("[中/英]切換"))), FALSE, FALSE, 0);

  GtkWidget *hbox_gtab_unique_auto_send = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_r), hbox_gtab_unique_auto_send, FALSE, FALSE, 0);
  GtkWidget *label_gtab_unique_auto_send = gtk_label_new(_(_L("唯一選擇時自動送出")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_unique_auto_send), label_gtab_unique_auto_send,  FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox_gtab_unique_auto_send),  create_auto_select_by_phrase_opts(&opt_gtab_unique_auto_send, gtab_unique_auto_send),  FALSE, FALSE, 0);


  GtkWidget *hbox_gtab_que_wild_card = gtk_hbox_new (FALSE, SPC);
  gtk_box_pack_start (GTK_BOX (vbox_gtab_r), hbox_gtab_que_wild_card, FALSE, FALSE, 0);
  GtkWidget *label_gtab_que_wild_card = gtk_label_new(_(_L("使用？萬用字元")));
  gtk_box_pack_start (GTK_BOX (hbox_gtab_que_wild_card), label_gtab_que_wild_card,  FALSE, FALSE, 0);
  check_button_gtab_que_wild_card = gtk_check_button_new ();
  gtk_box_pack_start (GTK_BOX (hbox_gtab_que_wild_card), check_button_gtab_que_wild_card,  FALSE, FALSE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button_gtab_que_wild_card),
     gtab_que_wild_card);


  gtk_box_pack_start (GTK_BOX (vbox_gtab_r), create_tsin_space_opts(), FALSE, FALSE, 0);

  GtkWidget *button_edit_append = gtk_button_new_with_label(_(_L("編輯內定輸入法的使用者外加字詞")));
  gtk_box_pack_start (GTK_BOX (vbox_gtab_r), button_edit_append, FALSE, FALSE, 0);

  g_signal_connect_swapped (G_OBJECT (button_edit_append), "clicked",
                            G_CALLBACK (cb_gtab_edit_append), NULL);


  GtkWidget *hbox_cancel_ok = gtk_hbox_new (FALSE, 10);
  gtk_box_pack_start (GTK_BOX (vbox_top), hbox_cancel_ok, FALSE, FALSE, 0);

  GtkWidget *button_cancel = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  if (button_order)
    gtk_box_pack_end (GTK_BOX (hbox_cancel_ok), button_cancel, TRUE, TRUE, 0);
  else
    gtk_box_pack_start (GTK_BOX (hbox_cancel_ok), button_cancel, TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (button_cancel), "clicked",
                            G_CALLBACK (close_gtab_conf_window),
                            G_OBJECT (gcin_gtab_conf_window));

  GtkWidget *button_ok = gtk_button_new_from_stock (GTK_STOCK_OK);

  if (button_order)
    gtk_box_pack_end (GTK_BOX (hbox_cancel_ok), button_ok, TRUE, TRUE, 0);
  else
    gtk_box_pack_start (GTK_BOX (hbox_cancel_ok), button_ok, TRUE, TRUE, 0);

  g_signal_connect_swapped (G_OBJECT (button_ok), "clicked",
                            G_CALLBACK (cb_gtab_conf_ok),
                            G_OBJECT (gcin_gtab_conf_window));

  GTK_WIDGET_SET_FLAGS (button_ok, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button_ok);


  GtkWidget *button_help = gtk_button_new_from_stock (GTK_STOCK_HELP);
  gtk_box_pack_end (GTK_BOX (hbox_cancel_ok), button_help, FALSE, FALSE, 0);

  g_signal_connect (G_OBJECT (button_help), "clicked",
                            G_CALLBACK (cb_gtab_help),
                            G_OBJECT (gcin_gtab_conf_window));


  gtk_widget_show_all (gcin_gtab_conf_window);

  return;
}
