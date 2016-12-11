#include "gcin.h"
#include "gtab.h"
#if UNIX
#include <signal.h>
#include <X11/extensions/XTest.h>
#if !GTK_CHECK_VERSION(2,16,0)
#include <X11/XKBlib.h>
#include <gdk/gdkx.h>
#define gdk_keymap_get_caps_lock_state(x) get_caps_lock_state()
#endif
#endif
#include "gst.h"
#include "pho.h"
#include "im-client/gcin-im-client-attr.h"
#include "win1.h"
#include "gcin-module.h"
#include "gcin-module-cb.h"

#define STRBUFLEN 64

extern Display *dpy;
#if USE_XIM
extern XIMS current_ims;
static IMForwardEventStruct *current_forward_eve;
#endif
extern gboolean win_kbm_inited;

static char *callback_str_buffer;
Window focus_win;
static int timeout_handle;
char *output_buffer;
int output_bufferN;
static char *output_buffer_raw, *output_buffer_raw_bak;
static int output_buffer_rawN;
#if WIN32
gboolean test_mode;
int last_input_method;
#endif
void set_wselkey();
void gtab_set_win1_cb();
void toggle_symbol_table();

gboolean old_capslock_on;


void init_gtab(int inmdno);

char current_method_type()
{
//  dbg("default_input_method %d\n",default_input_method);
  if (!current_CS)
#if UNIX
    return inmd[default_input_method].method_type;
#else
  {
    if (!last_input_method)
      last_input_method = default_input_method;
    return inmd[last_input_method].method_type;
  }
#endif

//  dbg("current_CS->in_method %d\n", current_CS->in_method);
  return inmd[current_CS->in_method].method_type;
}


#if WIN32
void win32_FakeKey(UINT vk, bool key_pressed);
#endif
void send_fake_key_eve(KeySym key)
{
#if WIN32
  win32_FakeKey(key, true);
  Sleep(10);
  win32_FakeKey(key, false);
#else
  KeyCode kc = XKeysymToKeycode(dpy, key);
  XTestFakeKeyEvent(dpy, kc, True, CurrentTime);
  usleep(10000);
  XTestFakeKeyEvent(dpy, kc, False, CurrentTime);
#endif
}

void fake_shift()
{
#if 0
  send_fake_key_eve(XK_Control_L);
#else
  send_fake_key_eve(XK_Shift_L);
#endif
}

void swap_ptr(char **a, char **b)
{
  char *t = *a;
  *a = *b;
  *b = t;
}

int force_preedit=0;
void force_preedit_shift()
{
  fake_shift();
  force_preedit=1;
}

void send_text_call_back(char *text)
{
  callback_str_buffer = (char *)realloc(callback_str_buffer, strlen(text)+1);
  strcpy(callback_str_buffer, text);
  fake_shift();
}

void output_buffer_call_back()
{
  swap_ptr(&callback_str_buffer, &output_buffer);

  if (output_buffer)
    output_buffer[0] = 0;
  output_bufferN = 0;

  fake_shift();
}

ClientState *current_CS;
static ClientState temp_CS;

void save_CS_current_to_temp()
{
#if UNIX
  if (!gcin_single_state)
    return;

//  dbg("save_CS_current_to_temp\n");
  temp_CS.b_half_full_char = current_CS->b_half_full_char;
  temp_CS.im_state = current_CS->im_state;
  temp_CS.in_method = current_CS->in_method;
  temp_CS.tsin_pho_mode = current_CS->tsin_pho_mode;
#endif
}


void save_CS_temp_to_current()
{
#if UNIX
  if (!gcin_single_state)
    return;

//  dbg("save_CS_temp_to_current\n");
  current_CS->b_half_full_char = temp_CS.b_half_full_char;
  current_CS->im_state = temp_CS.im_state;
  current_CS->in_method = temp_CS.in_method;
  current_CS->tsin_pho_mode = temp_CS.tsin_pho_mode;
#endif
}


gboolean init_in_method(int in_no);


void clear_output_buffer()
{
  if (output_buffer)
    output_buffer[0] = 0;
  output_bufferN = 0;

  swap_ptr(&output_buffer_raw, &output_buffer_raw_bak);

  if (output_buffer_raw)
    output_buffer_raw[0] = 0;
  output_buffer_rawN = 0;
}

gboolean gb_output = FALSE;

void toggle_gb_output()
{
  gb_output = !gb_output;
}

static void append_str(char **buf, int *bufN, char *text, int len)
{
  int requiredN = len + 1 + *bufN;
  *buf = (char *)realloc(*buf, requiredN);
  (*buf)[*bufN] = 0;
  strcat(*buf, text);
  *bufN += len;
}

int trad2sim(char *str, int strN, char **out);
void add_ch_time_str(char *s);

void send_text(char *text)
{
#if WIN32
  if (test_mode)
    return;
#endif

  char *filter;

  if (!text)
    return;
  int len = strlen(text);

  add_ch_time_str(text);

  append_str(&output_buffer_raw, &output_buffer_rawN, text, len);

  char *utf8_gbtext = NULL;

  if (gb_output) {
    len = trad2sim(text, len, &utf8_gbtext);
    text = utf8_gbtext;
  }

direct:
#if UNIX
  filter = getenv("GCIN_OUTPUT_FILTER");
  char filter_text[512];

  if (filter) {
    int pfdr[2], pfdw[2];

    if (pipe(pfdr) == -1) {
      dbg("cannot pipe r\n");
      goto next;
    }

    if (pipe(pfdw) == -1) {
      dbg("cannot pipe w\n");
      goto next;
    }

    int pid = fork();

    if (pid < 0) {
      dbg("cannot fork filter\n");
      goto next;
    }

    if (pid) {
      close(pfdw[0]);
      close(pfdr[1]);
      if (write(pfdw[1], text, len) < 0) {
      }
      close(pfdw[1]);
      int rn = read(pfdr[0], filter_text, sizeof(filter_text) - 1);
      filter_text[rn] = 0;
//      puts(filter_text);
      close(pfdr[0]);
      text = filter_text;
      len = rn;
    } else {
      close(pfdr[0]);
      close(pfdw[1]);
      dup2(pfdw[0], 0);
      dup2(pfdr[1], 1);
      if (execl(filter, filter, NULL) < 0) {
        dbg("execl %s err", filter);
        goto next;
      }
    }

  }
#endif
next:
  if (len) {
    append_str(&output_buffer, &output_bufferN, text, len);
  }

  free(utf8_gbtext);
}

void send_output_buffer_bak()
{
  send_text(output_buffer_raw_bak);
}

void set_output_buffer_bak_to_clipboard()
{
  char *text, *utf8_gbtext=NULL;

  if (gb_output) {
    trad2sim(output_buffer_raw_bak, strlen(output_buffer_raw_bak),
      &utf8_gbtext);
    text = utf8_gbtext;
  } else
    text = output_buffer_raw_bak;

#if UNIX && 0
  GtkClipboard *pclipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
#else
  GtkClipboard *pclipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
#endif

  gtk_clipboard_set_text(pclipboard, text, -1);

  free(utf8_gbtext);
}

void send_utf8_ch(char *bchar)
{
  char tt[CH_SZ+1];
  int len = utf8_sz(bchar);

  memcpy(tt, bchar, len);
  tt[len]=0;

  send_text(tt);
}


void send_ascii(char key)
{
  send_utf8_ch(&key);
}

#if USE_XIM
void export_text_xim()
{
  char *text = output_buffer;

  if (!output_bufferN)
    return;

  XTextProperty tp;
#if 0
  char outbuf[512];
  utf8_big5(output_buffer, outbuf);
  text = outbuf;
  XmbTextListToTextProperty(dpy, &text, 1, XCompoundTextStyle, &tp);
#else
  Xutf8TextListToTextProperty(dpy, &text, 1, XCompoundTextStyle, &tp);
#endif

#if DEBUG && 0
  dbg("send_utf8_ch: %s\n", text);
#endif

  ((IMCommitStruct*)current_forward_eve)->flag |= XimLookupChars;
  ((IMCommitStruct*)current_forward_eve)->commit_string = (char *)tp.value;
  IMCommitString(current_ims, (XPointer)current_forward_eve);

  clear_output_buffer();

  XFree(tp.value);
}


static void bounce_back_key()
{
    IMForwardEventStruct forward_ev = *(current_forward_eve);
    IMForwardEvent(current_ims, (XPointer)&forward_ev);
}
#endif

void hide_win0();
void hide_win_gtab();
void hide_win_pho();

int current_in_win_x = -1, current_in_win_y = -1;  // request x/y

void reset_current_in_win_xy()
{
#if 0
  current_in_win_x = current_in_win_y = -1;
#endif
}

GCIN_module_callback_functions *module_cb1(ClientState *cs)
{
  return inmd[cs->in_method].mod_cb_funcs;
}

GCIN_module_callback_functions *module_cb()
{
  if (!current_CS)
    return NULL;
  return module_cb1(current_CS);
}

void hide_in_win(ClientState *cs)
{
  if (!cs) {
#if 0
    dbg("hide_in_win: ic is null\n");
#endif
    return;
  }
#if 0
  dbg("hide_in_win %d\n", ic->in_method);
#endif

  if (timeout_handle) {
    g_source_remove(timeout_handle);
    timeout_handle = 0;
  }

  switch (current_method_type()) {
    case method_type_PHO:
      hide_win_pho();
      break;
#if USE_TSIN
    case method_type_TSIN:
//      flush_tsin_buffer();
      hide_win0();
      break;
#endif
    case method_type_MODULE:
      if (inmd[cs->in_method].mod_cb_funcs)
        module_cb1(cs)->module_hide_win();
      break;
    default:
      hide_win_gtab();
  }

  reset_current_in_win_xy();
}

void show_win_pho();
void show_win0();
void show_win_gtab();
void disp_tray_icon();

void check_CS()
{
  if (!current_CS) {
//    dbg("!current_CS");
    current_CS = &temp_CS;
//    temp_CS.input_style = InputStyleOverSpot;
//    temp_CS.im_state = GCIN_STATE_CHINESE;
#if TRAY_ENABLED
    disp_tray_icon();
#endif
  }
  else {
#if 0
    if (gcin_single_state)
      save_CS_temp_to_current();
    else
#endif
      temp_CS = *current_CS;
  }
}

gboolean force_show;

void show_in_win(ClientState *cs)
{
  if (!cs) {
#if 0
    dbg("show_in_win: ic is null");
#endif
    return;
  }

  switch (current_method_type()) {
    case method_type_PHO:
      show_win_pho();
      break;
#if USE_TSIN
    case method_type_TSIN:
      show_win0();
      break;
#endif
    case method_type_MODULE:
      if (!module_cb1(cs))
        return;
      module_cb1(cs)->module_show_win();
      break;
    default:
      show_win_gtab();
  }
#if 0
  show_win_stautus();
#endif
}


void move_win_gtab(int x, int y);
void move_win0(int x, int y);
void move_win_pho(int x, int y);

void move_in_win(ClientState *cs, int x, int y)
{
  check_CS();

  if (current_CS && current_CS->fixed_pos) {
    x = current_CS->fixed_x;
    y = current_CS->fixed_y;
  } else
  if (gcin_input_style == InputStyleRoot) {
    x = gcin_root_x;
    y = gcin_root_y;
  }

#if 0
  dbg("move_in_win %d %d\n",x, y);
#endif
#if 0
  if (current_in_win_x == x && current_in_win_y == y)
    return;
#endif
  current_in_win_x = x ; current_in_win_y = y;

  switch (current_method_type()) {
    case method_type_PHO:
      move_win_pho(x, y);
      break;
    case method_type_TSIN:
      move_win0(x, y);
      break;
    case method_type_MODULE:
      if (inmd[cs->in_method].mod_cb_funcs)
        module_cb1(cs)->module_move_win(x, y);
      break;
    default:
      if (!cs->in_method)
        return;
      move_win_gtab(x, y);
  }
}
#if UNIX
static int xerror_handler(Display *d, XErrorEvent *eve)
{
  return 0;
}
#endif

void getRootXY(Window win, int wx, int wy, int *tx, int *ty)
{
  if (!win) {
	*tx = wx;
	*ty = wy;
	return;
  }

#if WIN32
  POINT pt;
  pt.x = wx; pt.y = wy;
  ClientToScreen((HWND)win, &pt);
  *tx = pt.x; *ty=pt.y;
#else
  Window ow;
  XErrorHandler olderr = XSetErrorHandler((XErrorHandler)xerror_handler);
  XTranslateCoordinates(dpy,win,root,wx,wy,tx,ty,&ow);
  XSetErrorHandler(olderr);
#endif
}

extern int dpy_x_ofs, dpy_y_ofs;

void move_IC_in_win(ClientState *cs)
{
#if 0
   dbg("move_IC_in_win %d,%d\n", cs->spot_location.x, cs->spot_location.y);
#endif
   Window inpwin = cs->client_win;
#if UNIX
   if (!inpwin) {
     dbg("no inpwin %p\n", cs);
     return;
   }
#endif
   // non focus win filtering is done in the client lib
   if (inpwin != focus_win && focus_win && !cs->b_gcin_protocol) {
      return;
   }

   int inpx = cs->spot_location.x;
   int inpy = cs->spot_location.y;

#if UNIX
   XWindowAttributes att;
   XGetWindowAttributes(dpy, inpwin, &att);
// chrome window is override_redirect
//   if (att.override_redirect)
//     return;

   if (inpx >= att.width)
     inpx = att.width - 1;
   if (inpy >= att.height)
     inpy = att.height - 1;
#else
   if (inpwin) {
     RECT rect;
     GetClientRect((HWND)inpwin, &rect);

	 rect.right -= dpy_x_ofs;
	 rect.bottom -= dpy_y_ofs;

     if (inpx >= rect.right)
       inpx = rect.right - 1;
     if (inpy >= rect.bottom)
       inpy = rect.bottom - 1;
   }
//   dbg("GetClientRect %x %d,%d\n", inpwin, inpx, inpy);
#endif
   int tx,ty;
   getRootXY(inpwin, inpx, inpy, &tx, &ty);

#if 0
   dbg("move_IC_in_win inpxy:%d,%d txy:%d,%d\n", inpx, inpy, tx, ty);
#endif

   move_in_win(cs, tx, ty+1);
}


void update_in_win_pos()
{
  check_CS();

//  dbg("update_in_win_pos %x %d\n", current_CS, current_CS->input_style);

  if (current_CS->input_style == InputStyleRoot) {
#if UNIX
    Window r_root, r_child;
    int winx, winy, rootx, rooty;
    u_int mask;

    XQueryPointer(dpy, root, &r_root, &r_child, &rootx, &rooty, &winx, &winy, &mask);

    winx++; winy++;

    Window inpwin = current_CS->client_win;
#if 0
    dbg("update_in_win_pos\n");
#endif
    if (inpwin) {
      int tx, ty;
      Window ow;

      XTranslateCoordinates(dpy, root, inpwin, winx, winy, &tx, &ty, &ow);

      current_CS->spot_location.x = tx;
      current_CS->spot_location.y = ty;
    }
#else
	  int winx=0, winy=0;
      current_CS->spot_location.x = 0;
      current_CS->spot_location.y = 0;
#endif

    move_in_win(current_CS, winx, winy);
  } else {
    move_IC_in_win(current_CS);
  }

  disp_tray_icon();
}

void win_pho_disp_half_full();
void win_tsin_disp_half_full();
void win_gtab_disp_half_full();
void update_tray_icon(), load_tray_icon(), load_tray_icon_win32();
static int current_gcin_win32_icon = -1;
void restart_gcin0();
extern void destroy_tray_win32();
extern void destroy_tray_icon();

#if TRAY_ENABLED
#if UNIX
void destroy_tray()
{
  if (current_gcin_win32_icon==GCIN_TRAY_WIN32)
    destroy_tray_win32();
  else
  if (current_gcin_win32_icon==GCIN_TRAY_UNIX)
    destroy_tray_icon();
#if USE_INDICATOR
  else
    destroy_tray_indicator();
#endif
}
#endif

void disp_tray_icon()
{
//  dbg("disp_tray_icon\n");
//dbg("disp_tray_icon %d %d\n", current_gcin_win32_icon, gcin_win32_icon);
#if UNIX
  if (current_gcin_win32_icon >= 0 && current_gcin_win32_icon != gcin_win32_icon) {
    destroy_tray();
  }

  current_gcin_win32_icon = gcin_win32_icon;

  if (gcin_win32_icon==GCIN_TRAY_WIN32)
#endif
    load_tray_icon_win32();
#if UNIX
  else
  if (gcin_win32_icon==GCIN_TRAY_UNIX)
    load_tray_icon();
#if USE_INDICATOR
  else
    load_tray_icon_indicator();
#endif
#endif
}
#endif

void disp_im_half_full()
{
//  dbg("disp_im_half_full\n");
#if TRAY_ENABLED
  disp_tray_icon();
#endif

  switch (current_method_type()) {
    case method_type_PHO:
      win_pho_disp_half_full();
      break;
#if USE_TSIN
    case method_type_TSIN:
      win_tsin_disp_half_full();
      break;
#endif
    default:
      win_gtab_disp_half_full();
      break;
  }
}

void flush_tsin_buffer();
void reset_gtab_all();
void set_tsin_pho_mode0(ClientState *cs, gboolean tsin_pho_mode);

//static u_int orig_caps_state;

void init_state_chinese(ClientState *cs, gboolean tsin_pho_mode)
{
  dbg("init_state_chinese %p\n",cs);

  cs->im_state = GCIN_STATE_CHINESE;
  set_tsin_pho_mode0(cs, tsin_pho_mode);
  if (!cs->in_method)
#if UNIX
    init_in_method2(cs, default_input_method);
#else
  if (!last_input_method)
    last_input_method = default_input_method;
  init_in_method(last_input_method);
#endif

  save_CS_current_to_temp();
}

gboolean output_gbuf();
void update_win_kbm();
void flush_edit_buffer(), ClrIn();
void tsin_set_eng_ch(int nmod);
int current_kbd_state;

void toggle_im_enabled()
{
//    dbg("toggle_im_enabled\n");
    check_CS();

    if (current_CS->in_method < 0)
      p_err("err found");

    int status=0;

//  dbg("feedkey_pp %x %x\n", xkey, kbstate);
//  if (xkey=='1')
//    dbg("aaa\n");


    if (current_CS->im_state != GCIN_STATE_DISABLED) {
      if (current_CS->im_state == GCIN_STATE_ENG_FULL) {
        current_CS->im_state = GCIN_STATE_CHINESE;
        disp_im_half_full();
        save_CS_current_to_temp();
        return;
      }

      flush_edit_buffer();

      hide_in_win(current_CS);
#if 0
      hide_win_status();
#endif
      current_CS->im_state = GCIN_STATE_DISABLED;

      update_win_kbm();

      disp_tray_icon();
    } else {
      if (!current_method_type())
        init_gtab(current_CS->in_method);

	  ClrIn();

      init_state_chinese(current_CS, TRUE);
      reset_current_in_win_xy();
#if 1
      show_in_win(current_CS);
      update_in_win_pos();
#else
      update_in_win_pos();
      show_in_win(current_CS);
#endif

      update_win_kbm();

      if (tsin_chinese_english_toggle_key == TSIN_CHINESE_ENGLISH_TOGGLE_KEY_CapsLock) {
        tsin_set_eng_ch(!(current_kbd_state&LockMask));
      }

      disp_tray_icon();
    }

    save_CS_current_to_temp();
}

void get_win_gtab_geom();
void get_win0_geom();
void get_win_pho_geom();

void update_active_in_win_geom()
{
//  dbg("update_active_in_win_geom\n");
  switch (current_method_type()) {
    case method_type_PHO:
      get_win_pho_geom();
      break;
#if USE_TSIN
    case method_type_TSIN:
      get_win0_geom();
      break;
#endif
    case method_type_MODULE:
      if (module_cb() && module_cb()->module_get_win_geom)
        module_cb()->module_get_win_geom();
      break;
    default:
      get_win_gtab_geom();
      break;
  }
}

extern GtkWidget *gwin_pho, *gwin0, *gwin_gtab;

gboolean win_is_visible()
{
  if (!current_CS)
    return FALSE;
  switch (current_method_type()) {
    case method_type_PHO:
      return gwin_pho && GTK_WIDGET_VISIBLE(gwin_pho);
#if USE_TSIN
    case method_type_TSIN:
      return gwin0 && GTK_WIDGET_VISIBLE(gwin0);
#endif
    case method_type_MODULE:
      if (!module_cb())
        return FALSE;
      return module_cb()->module_win_visible();
    default:
      if (!gwin_gtab)
        return FALSE;
      return gwin_gtab && GTK_WIDGET_VISIBLE(gwin_gtab);
  }

  return FALSE;
}


void disp_gtab_half_full(gboolean hf);
void tsin_toggle_half_full();

void toggle_half_full_char_sub()
{
  if (current_method_type() == method_type_TSIN && current_CS->im_state == GCIN_STATE_CHINESE) {
    tsin_toggle_half_full();
  }
  else {
    if (current_CS->im_state == GCIN_STATE_ENG_FULL) {
      current_CS->im_state = GCIN_STATE_DISABLED;
      hide_in_win(current_CS);
    } else
    if (current_CS->im_state == GCIN_STATE_DISABLED) {
      toggle_im_enabled();
      current_CS->im_state = GCIN_STATE_ENG_FULL;
    } else
    if (current_CS->im_state == GCIN_STATE_CHINESE) {
      current_CS->b_half_full_char = !current_CS->b_half_full_char;
    }

//    dbg("current_CS->in_method %d\n", current_CS->in_method);
    disp_im_half_full();
  }

  save_CS_current_to_temp();
}

void toggle_half_full_char()
{
#if WIN32
  if (test_mode)
    return;
#endif

  check_CS();

  if (!gcin_shift_space_eng_full) {
    current_CS->b_half_full_char = 0;
    tss.tsin_half_full=0;
    disp_im_half_full();
    return;
  }

//  dbg("toggle_half_full_char\n");
  toggle_half_full_char_sub() ;
}

void init_tab_pp(gboolean init);
void init_tab_pho();


extern int b_show_win_kbm;

void hide_win_kbm();
extern gboolean win_sym_enabled;
void show_win_kbm();
extern char *TableDir;
void set_gtab_input_method_name(char *s);
GCIN_module_callback_functions *init_GCIN_module_callback_functions(char *sofile);
time_t find_tab_file(char *fname, char *out_file);
gboolean tsin_page_up(), tsin_page_down();
int tsin_sele_by_idx(int);
void free_tsin();

void tsin_set_win1_cb()
{
  set_win1_cb((cb_selec_by_idx_t)tsin_sele_by_idx, (cb_page_ud_t)tsin_page_up, (cb_page_ud_t)tsin_page_down);
}

void update_win_kbm_inited()
{
  if (win_kbm_inited)
    update_win_kbm();
}

gboolean init_in_method2(ClientState *cs, int in_no)
{
  dbg("init_in_method2 %p %d\n", cs, in_no);
  gboolean init_im = !(cur_inmd && (cur_inmd->flag & FLAG_GTAB_SYM_KBM));

  if (in_no < 0)
    return FALSE;

  if (cs==current_CS && cs->in_method != in_no) {
    if (!(inmd[in_no].flag & FLAG_GTAB_SYM_KBM)) {
      flush_edit_buffer();

      hide_in_win(cs);
    }

    if (cur_inmd && (cur_inmd->flag & FLAG_GTAB_SYM_KBM))
      hide_win_kbm();
  }

  if (cs==current_CS)
    reset_current_in_win_xy();

//  dbg("switch init_in_method %x %d\n", current_CS, in_no);
//  set_tsin_pho_mode0(cs, ini_tsin_pho_mode);
  tsin_set_win1_cb();

  switch (inmd[in_no].method_type) {
    case method_type_PHO:
      cs->in_method = in_no;
      init_tab_pho();
      break;
    case method_type_TSIN:
	  free_tsin();
      set_wselkey();
      cs->in_method = in_no;
//      if (cs==current_CS || !current_CS)
		init_tab_pp(init_im);
      break;
    case method_type_SYMBOL_TABLE:
      toggle_symbol_table();
      break;
    case method_type_MODULE:
    {
      GCIN_module_main_functions gmf;
      init_GCIN_module_main_functions(&gmf);
      if (!inmd[in_no].mod_cb_funcs) {
        char ttt[256];
        strcpy(ttt, inmd[in_no].filename);

        dbg("module %s\n", ttt);
        if (!(inmd[in_no].mod_cb_funcs = init_GCIN_module_callback_functions(ttt))) {
          dbg("module not found\n");
          return FALSE;
        }
      }

      if ((cs==current_CS || !current_CS) && inmd[in_no].mod_cb_funcs->module_init_win(&gmf)) {
        cs->in_method = in_no;
        module_cb()->module_show_win();
        if (cs==current_CS)
          set_wselkey();
      } else {
        return FALSE;
      }

      break;
    }
    case method_type_EN:
    {
      if (cs==current_CS && current_CS->im_state==GCIN_STATE_CHINESE)
        toggle_im_enabled();
      return TRUE;
    }
    case method_type_GTAB:
    {
      dbg("method_type_GTAB\n");
	  free_tsin();
      if (cs==current_CS || !current_CS)
		init_gtab(in_no);

      if (!inmd[in_no].DefChars)
        return FALSE;
      cs->in_method = in_no;
      if (!(inmd[in_no].flag & FLAG_GTAB_SYM_KBM))
        show_win_gtab();
      else {
        win_kbm_inited = 1;
        show_win_kbm();
      }

      set_gtab_input_method_name(inmd[in_no].cname);
      break;
    }
    default:
      dbg("unknown\n");
      return FALSE;
  }
#if WIN32
  if (cs==current_CS && current_CS->in_method != last_input_method)
    last_input_method = current_CS->in_method;
#endif

  if (cs==current_CS) {
	char *selkey;
    if (selkey=inmd[current_CS->in_method].selkey) {
      set_wselkey();
      gtab_set_win1_cb();
  //    dbg("aa selkey %s\n", inmd[current_CS->in_method].selkey);
    }
    update_in_win_pos();
    update_win_kbm_inited();
    disp_tray_icon();
  }

  return TRUE;
}

gboolean init_in_method(int in_no)
{
  check_CS();
  return init_in_method2(current_CS, in_no);
}

static void cycle_next_in_method()
{
  int i;
  dbg("cycle_next_in_method\n");
#if WIN32
  if (test_mode)
    return;
#endif

  for(i=0; i < inmdN; i++) {
    int v = (current_CS->in_method + 1 + i) % inmdN;
    if (!inmd[v].in_cycle)
      continue;
    if (!inmd[v].cname || !inmd[v].cname[0])
      continue;

    if (!init_in_method(v))
      continue;

    return;
  }
}

void add_to_tsin_buf_str(char *str);
gboolean gtab_phrase_on();
void insert_gbuf_nokey(char *s);

gboolean full_char_proc(KeySym keysym)
{
  char *s = half_char_to_full_char(keysym);

  if (!s)
    return 0;

  char tt[CH_SZ+1];

  utf8cpy(tt, s);

  if (current_CS->im_state  == GCIN_STATE_ENG_FULL) {
    send_text(tt);
    return 1;
  }

  if (current_method_type() == 6 && current_CS->im_state == GCIN_STATE_CHINESE)
    add_to_tsin_buf_str(tt);
  else
  if (gtab_phrase_on() && ggg.gbufN)
    insert_gbuf_nokey(tt);
  else
    send_text(tt);

  return 1;
}

int feedkey_pho(KeySym xkey, int kbstate);
int feedkey_tsin(KeySym xkey, int state);
int feedkey_gtab(KeySym key, int kbstate);
int feed_phrase(KeySym ksym, int state);
void tsin_set_eng_ch(int nmod);
static KeySym last_keysym;

gboolean timeout_raise_window(gpointer data)
{
//  dbg("timeout_raise_window\n");
  timeout_handle = 0;
  show_in_win(current_CS);
  return FALSE;
}

extern Window xwin_pho, xwin0, xwin_gtab;
void create_win_sym(), win_kbm_disp_caplock();

#if !GTK_CHECK_VERSION(2,16,0) && 0
#include <X11/extensions/XKBsrv.h>
gboolean get_caps_lock_state()
{
  XkbStateRec states;

  if (XkbGetState(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), XkbUseCoreKbd, &states) == Success) {
    if (states.locked_mods & LockMask) return TRUE;
  }
  return FALSE;
}
#endif

gboolean capslock_on;

void disp_win_kbm_capslock()
{
  if (!b_show_win_kbm)
    return;

  gboolean o_state = old_capslock_on;
  old_capslock_on = capslock_on;

//  dbg("%x %x\n", old_capslock_on, o_state);

  if (o_state != old_capslock_on) {
    win_kbm_disp_caplock();
  }
}


void disp_win_kbm_capslock_init()
{
  old_capslock_on = gdk_keymap_get_caps_lock_state(gdk_keymap_get_default());
//  dbg("disp_win_kbm_capslock_init %d\n",old_capslock_on);

  if (b_show_win_kbm)
    win_kbm_disp_caplock();
}

void toggle_symbol_table()
{
  if (current_CS->im_state == GCIN_STATE_CHINESE) {
    if (!win_is_visible())
      win_sym_enabled=1;
    else
      win_sym_enabled^=1;
  } else
    win_sym_enabled=0;

  create_win_sym();
  if (win_sym_enabled) {
    force_show = TRUE;
    if (current_CS->im_state == GCIN_STATE_CHINESE)
      show_in_win(current_CS);
    force_show = FALSE;
  }
}


void destroy_phrase_save_menu();
int gcin_switch_keys_lookup(int key);
gboolean b_menu_key_pressed;
void cb_trad_sim_toggle();
void set_tsin_pho_mode();

// return TRUE if the key press is processed
gboolean ProcessKeyPress(KeySym keysym, u_int kev_state)
{
#if 1
  dbg("key press %x %x\n", keysym, kev_state);
#endif

  current_kbd_state = kev_state;
  destroy_phrase_save_menu();

  capslock_on = (kev_state&LockMask) > 0;
  if (keysym == XK_Caps_Lock) {
#if UNIX
    capslock_on = !(kev_state&LockMask);
#endif
    disp_tray_icon();
  }

  disp_win_kbm_capslock();
  check_CS();

  if (current_CS->client_win)
    focus_win = current_CS->client_win;

  if (
#if WIN32
  !test_mode &&
#endif
  callback_str_buffer && strlen(callback_str_buffer)) {
    send_text(callback_str_buffer);
    callback_str_buffer[0]=0;
    return TRUE;
  }

  if (force_preedit==1) {
	dbg("orce_preedit==1\n");
    force_preedit = 2;
    return 1;
  }

#if 1
  // Chrome win32 has this problem
  if (keysym==XK_Menu) {
    b_menu_key_pressed = TRUE;
  } else {
    gboolean old_pressed = b_menu_key_pressed;
    b_menu_key_pressed = FALSE;
    if (old_pressed && strchr("utcpda", keysym))
      return FALSE;
  }
#endif


  if (keysym == XK_space) {
#if 0
    dbg("state %x\n", kev->state);
    dbg("%x\n", Mod4Mask);
#endif
    if (
      ((kev_state & (ControlMask|Mod1Mask|ShiftMask))==ControlMask && gcin_im_toggle_keys==Control_Space) ||
      ((kev_state & Mod1Mask) && gcin_im_toggle_keys==Alt_Space) ||
      ((kev_state & ShiftMask) && gcin_im_toggle_keys==Shift_Space) ||
      ((kev_state & Mod4Mask) && gcin_im_toggle_keys==Windows_Space)
    ) {
	  dbg("ctrl-space\n");

      if (
#if 0
		  current_method_type() == method_type_TSIN &&
#endif
		  tsin_chinese_english_toggle_key != TSIN_CHINESE_ENGLISH_TOGGLE_KEY_CapsLock) {
        tsin_set_eng_ch(1);
      }

      toggle_im_enabled();
#if UNIX
      return TRUE;
#else
      return FALSE;
#endif
    }
  }

  if (keysym == XK_space && (kev_state & ShiftMask)) {
    if (last_keysym != XK_Shift_L && last_keysym != XK_Shift_R)
      return FALSE;

    toggle_half_full_char();

    return TRUE;
  }


  if (((kev_state & (Mod1Mask|ShiftMask)) == (Mod1Mask|ShiftMask)) ||
	  (gcin_ctrl_punc && (kev_state&(Mod1Mask|Mod5Mask|ShiftMask|ControlMask))==ControlMask && keysym < 127 && !(keysym>='0' && keysym<='9')) ) {
    if (current_CS->im_state != GCIN_STATE_DISABLED || gcin_eng_phrase_enabled)
      return feed_phrase(keysym, kev_state);
    else
      return 0;
  }

//  dbg("state %x\n", kev_state);
  if ((current_CS->im_state & (GCIN_STATE_ENG_FULL)) ) {
    return full_char_proc(keysym);
  }


  if ((kev_state & ControlMask) && (kev_state&(Mod1Mask|Mod5Mask|ShiftMask))) {
    if (keysym == 'g' || keysym == 'r' || keysym == XK_F5) {
      last_keysym = 0;
      send_output_buffer_bak();
      return TRUE;
    }

    if (keysym == XK_F8) {
      last_keysym = 0;
      cb_trad_sim_toggle();
    }

    if (!gcin_enable_ctrl_alt_switch)
      return FALSE;

#if WIN32
	if (kev_state & ShiftMask) {
		struct {
			KeySym frm, to;
		} tkeys[]={{0x1e, '6'}};

		for(int i=0; tkeys[i].frm; i++) {
			if (tkeys[i].frm == keysym) {
				keysym = tkeys[i].to;
				break;
			}
		}
	}
#endif

    int kidx = gcin_switch_keys_lookup(keysym);
    last_keysym = keysym;
    if (kidx < 0) {
      return FALSE;
    }

    if (inmd[kidx].method_type == method_type_SYMBOL_TABLE) {
      toggle_symbol_table();
      return TRUE;
    }

    if (!inmd[kidx].cname)
      return FALSE;

    current_CS->im_state = GCIN_STATE_CHINESE;
#if WIN32
    if (!test_mode)
#endif
    {
      init_in_method(kidx);
      set_tsin_pho_mode();
    }

    return TRUE;
  }

  last_keysym = keysym;

  if (current_CS->im_state == GCIN_STATE_DISABLED) {
    return FALSE;
  }

#if 0
  if (!current_CS->b_gcin_protocol) {
  if (((keysym == XK_Control_L || keysym == XK_Control_R)
                   && (kev_state & ShiftMask)) ||
      ((keysym == XK_Shift_L || keysym == XK_Shift_R)
                   && (kev_state & ControlMask))) {
     cycle_next_in_method();
     return TRUE;
  }
  }
#endif

  if (current_CS->b_raise_window && keysym>=' ' && keysym < 127) {
    if (timeout_handle)
      g_source_remove(timeout_handle);
    timeout_handle = g_timeout_add(200, timeout_raise_window, NULL);
  }


  switch(current_method_type()) {
    case method_type_PHO:
      return feedkey_pho(keysym, kev_state);
    case method_type_TSIN:
      return feedkey_tsin(keysym, kev_state);
    case method_type_MODULE:
      if (!module_cb())
        return FALSE;
      return module_cb()->module_feedkey(keysym, kev_state);
    case method_type_GTAB:
      return feedkey_gtab(keysym, kev_state);
    default:
      dbg("ProcessKeyPress unknown current_CS:%x %d\n", current_CS, current_method_type());
  }

  return FALSE;
}

int feedkey_pp_release(KeySym xkey, int kbstate);
int feedkey_gtab_release(KeySym xkey, int kbstate);

// return TRUE if the key press is processed
gboolean ProcessKeyRelease(KeySym keysym, u_int kev_state)
{
  check_CS();
  current_kbd_state = kev_state;

  if (force_preedit==2) {
	dbg("orce_preedit==2\n");
	force_preedit = 0;
	return TRUE;
  }

  disp_win_kbm_capslock();

//  check_CS();
#if 0
  dbg_time("key release %x %x\n", keysym, kev_state);
#endif

  if (current_CS->im_state == GCIN_STATE_DISABLED)
    return FALSE;


#if 1
  dbg("last_keysym %x\n", last_keysym);

  if (current_CS->b_gcin_protocol && (last_keysym == XK_Shift_L ||
  last_keysym == XK_Shift_R || last_keysym == XK_Control_L || last_keysym == XK_Control_R)) {
    if (((keysym == XK_Control_L || keysym == XK_Control_R)
          && (kev_state & ShiftMask)) ||
        ((keysym == XK_Shift_L || keysym == XK_Shift_R)
          && (kev_state & ControlMask))) {
       cycle_next_in_method();
       return TRUE;
    }
  }
#endif

  switch(current_method_type()) {
    case method_type_TSIN:
      return feedkey_pp_release(keysym, kev_state);
    case method_type_MODULE:
      if (!module_cb())
        return FALSE;
      return module_cb()->module_feedkey_release(keysym, kev_state);
    case method_type_GTAB:
      return feedkey_gtab_release(keysym, kev_state);
  }

  return FALSE;
}


#if USE_XIM
int xim_ForwardEventHandler(IMForwardEventStruct *call_data)
{
    current_forward_eve = call_data;

    if (call_data->event.type != KeyPress && call_data->event.type != KeyRelease) {
#if DEBUG || 1
        dbg("bogus event type, ignored\n");
#endif
        return True;
    }

    char strbuf[STRBUFLEN];
    KeySym keysym;

    bzero(strbuf, STRBUFLEN);
    XKeyEvent *kev = (XKeyEvent*)&current_forward_eve->event;
    XLookupString(kev, strbuf, STRBUFLEN, &keysym, NULL);

    int ret;
    if (call_data->event.type == KeyPress)
      ret = ProcessKeyPress(keysym, kev->state);
    else
      ret = ProcessKeyRelease(keysym, kev->state);

    if (!ret)
      bounce_back_key();

    export_text_xim();

    return False;
}
#endif


void gcin_reset();

#if UNIX
gboolean is_tip_window(Window inpwin)
{
   // Dirty fix for chrome, doesn't work well.
   if (!inpwin)
     return FALSE;
   XWindowAttributes att;
   XGetWindowAttributes(dpy, inpwin, &att);

   dbg("%d, %d\n", att.width, att.height);
// chrome window is override_redirect
//   if (att.override_redirect)
//     return;

   return att.override_redirect && att.height < 24;
}
#endif

int gcin_FocusIn(ClientState *cs)
{
  dbg("gcin_FocusIn %p\n", cs);
  if (!cs) {
    dbg("cs is null");
    return FALSE;
  } else {
//    dbg("gcin_FocusIn %x\n", cs);
  }

  Window win = cs->client_win;
  dbg("win: %x\n", win);

  if (!win) {
	dbg("!win");
    return FALSE;
  }
#if UNIX && 0
  if (is_tip_window(win)) {
	dbg("is_tip_window\n");
	return FALSE;
  }
#endif
  reset_current_in_win_xy();

  if (cs) {
    Window win = cs->client_win;

    if (focus_win != win) {
#if 1
      dbg("reset %x,%x\n", focus_win, win);
      gcin_reset();
#endif
      hide_in_win(current_CS);
      focus_win = win;
    }
  }

  current_CS = cs;
  save_CS_temp_to_current();

//  dbg("current_CS %x %d %d\n", cs, cs->im_state, current_CS->im_state);

  if (win == focus_win) {
    if (cs->im_state != GCIN_STATE_DISABLED) {
      show_in_win(cs);
      move_IC_in_win(cs);
    } else {
      hide_in_win(cs);
      move_IC_in_win(cs);
    }
  }

  cur_inmd = &inmd[current_CS->in_method];

//  if (inmd[cs->in_method].selkey)
//    set_wselkey();
//  else
  {
    set_wselkey();
#if 0
    gtab_set_win1_cb();
    tsin_set_win1_cb();
#endif
  }

  update_win_kbm();

  disp_tray_icon();

#if 0
  dbg_time("gcin_FocusIn %x %x\n",cs, current_CS);
#endif
  return True;
}


#if USE_XIM
IC *FindIC(CARD16 icid);
void load_IC(IC *rec);
CARD16 connect_id;

int xim_gcin_FocusIn(IMChangeFocusStruct *call_data)
{
    IC *ic = FindIC(call_data->icid);
    ClientState *cs = &ic->cs;
    connect_id = call_data->connect_id;

    if (ic) {
      gcin_FocusIn(cs);

      load_IC(ic);
    }

#if DEBUG
    dbg("xim_gcin_FocusIn %d\n", call_data->icid);
#endif
    return True;
}
#endif



static gint64 last_focus_out_time;

int gcin_FocusOut(ClientState *cs)
{
  gint64 t = current_time();
//  dbg("gcin_FocusOut\n");

  if (cs != current_CS)
     return FALSE;

  if (!cs) {
	 dbg("gcin_FocusOut is null\n");
	 return FALSE;
  }

  if (!cs->client_win)
    return FALSE;

#if UNIX
  if (is_tip_window(cs->client_win)) {
	return FALSE;
  }
#endif


  if (t - last_focus_out_time < 100000) {
    last_focus_out_time = t;
    return FALSE;
  }

  last_focus_out_time = t;

  if (cs == current_CS) {
    hide_in_win(cs);
  }

  reset_current_in_win_xy();

  if (cs == current_CS)
    temp_CS = *current_CS;

#if 0
  dbg("focus out\n");
#endif

  return True;
}

int tsin_get_preedit(char *str, GCIN_PREEDIT_ATTR attr[], int *cursor, int *sub_comp_len);
int gtab_get_preedit(char *str, GCIN_PREEDIT_ATTR attr[], int *pcursor, int *sub_comp_len);
int int_get_preedit(char *str, GCIN_PREEDIT_ATTR attr[], int *cursor, int *sub_comp_len);
int pho_get_preedit(char *str, GCIN_PREEDIT_ATTR attr[], int *cursor, int *sub_comp_len);

int gcin_get_preedit(ClientState *cs, char *str, GCIN_PREEDIT_ATTR attr[], int *cursor, int *comp_flag)
{
//  dbg("gcin_get_preedit %x\n", current_CS);
  if (!current_CS) {
empty:
//    dbg("empty\n");
    str[0]=0;
    *cursor=0;
    return 0;
  }

  str[0]=0;
  *comp_flag=0;

//  cs->use_preedit = TRUE;

  switch(current_method_type()) {
    case method_type_PHO:
      return pho_get_preedit(str, attr, cursor, comp_flag);
#if USE_TSIN
    case method_type_TSIN:
      return tsin_get_preedit(str, attr, cursor, comp_flag);
#endif
    case method_type_MODULE:
      if (inmd[current_CS->in_method].mod_cb_funcs)
        return module_cb()->module_get_preedit(str, attr, cursor, comp_flag);
    default:
      return gtab_get_preedit(str, attr, cursor, comp_flag);
//      dbg("metho %d\n", current_CS->in_method);
  }

  return 0;
}

void pho_reset();
int tsin_reset();
void gtab_reset();

void gcin_reset()
{
#if 1
  if (!current_CS)
    return;
//  dbg("gcin_reset\n");

  switch(current_method_type()) {
    case method_type_PHO:
      pho_reset();
      return;
#if USE_TSIN
    case method_type_TSIN:
      tsin_reset();
      return;
#endif
    case method_type_MODULE:
      if (inmd[current_CS->in_method].mod_cb_funcs)
        module_cb()->module_reset();
      return;
    default:
      gtab_reset();
//      dbg("metho %d\n", current_CS->in_method);
  }
#endif
}

#if WIN32
void show_tsin_stat();
void gcin_set_tsin_pho_mode(ClientState *cs, gboolean pho_mode)
{
	set_tsin_pho_mode0(cs, pho_mode);

	if (!pho_mode) {
		cs->b_half_full_char = 0;
		tss.tsin_half_full = 0;
	}
	show_tsin_stat();
}
#endif

#if USE_XIM
int xim_gcin_FocusOut(IMChangeFocusStruct *call_data)
{
    IC *ic = FindIC(call_data->icid);
    ClientState *cs = &ic->cs;

    gcin_FocusOut(cs);

    return True;
}
#endif


gboolean gcin_edit_display_ap_only()
{
#if WIN32
  if (test_mode)
    return TRUE;
#endif
  if (!current_CS)
    return FALSE;
//  dbg("gcin_edit_display_ap_only %d\n", current_CS->use_preedit)
  return current_CS->use_preedit && gcin_edit_display==GCIN_EDIT_DISPLAY_ON_THE_SPOT;
}


gboolean gcin_display_on_the_spot_key()
{
  return gcin_edit_display_ap_only() && gcin_on_the_spot_key;
}

void flush_edit_buffer()
{
//  dbg("flush_edit_buffer\n");
  if (!current_CS)
    return;
//  dbg("gcin_reset\n");
  switch(current_method_type()) {
#if USE_TSIN
    case method_type_TSIN:
      flush_tsin_buffer();
      break;
#endif
    case method_type_MODULE:
      if (inmd[current_CS->in_method].mod_cb_funcs)
      module_cb()->module_flush_input();
      break;
    default:
      output_gbuf();
//      dbg("metho %d\n", current_CS->in_method);
  }
}

#if WIN32
void pho_save_gst(), tsin_save_gst(), gtab_save_gst();
void pho_restore_gst(), tsin_restore_gst(), gtab_restore_gst();

gboolean ProcessTestKeyPress(KeySym keysym, u_int kev_state)
{
  if (!current_CS)
    return TRUE;
//  dbg("gcin_reset\n");
  gboolean v;

  test_mode= TRUE;
  switch(current_method_type()) {
    case method_type_PHO:
      pho_save_gst();
      v = ProcessKeyPress(keysym, kev_state);
      pho_restore_gst();
      break;
    case method_type_TSIN:
      tsin_save_gst();
      v = ProcessKeyPress(keysym, kev_state);
      tsin_restore_gst();
      break;
    case method_type_MODULE:
      break;
    default:
      gtab_save_gst();
      v = ProcessKeyPress(keysym, kev_state);
      gtab_restore_gst();
  }

  test_mode= FALSE;

  return v;
}


gboolean Process2KeyPress(KeySym keysym, u_int kev_state)
{
  gboolean tv = ProcessTestKeyPress(keysym, kev_state);
  gboolean v = ProcessKeyPress(keysym, kev_state);

  if (tv != v)
    dbg("ProcessKeyPress %x -> %d %d\n",keysym, tv, v);

  return v;
}


gboolean ProcessTestKeyRelease(KeySym keysym, u_int kev_state)
{
  test_mode = TRUE;
  gboolean v = ProcessKeyRelease(keysym, kev_state);
  test_mode = FALSE;
  return v;
}


gboolean Process2KeyRelease(KeySym keysym, u_int kev_state)
{
  gboolean tv = ProcessTestKeyRelease(keysym, kev_state);
  gboolean v = ProcessKeyRelease(keysym, kev_state);

  if (tv != v)
    dbg("ProcessKeyRelease %x -> %d %d\n",keysym, tv, v);

  return v;
}
#endif
