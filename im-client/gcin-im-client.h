#ifndef GCIN_IM_CLIENT_H
#define GCIN_IM_CLIENT_H
struct GCIN_PASSWD;

typedef struct GCIN_client_handle_S {
#if UNIX
  int fd;
#else
  HANDLE fd;               // <=0 ; connection is not established
  int server_idx;
#endif
  Window client_win;	/* client window */
  u_int	input_style;	/* input style */
  XPoint spot_location; /* spot location */
// below is private data, don't modify them.
  u_int flag;
  Display *disp;
  struct GCIN_PASSWD *passwd;
  u_int seq;
} GCIN_client_handle;

enum {
  FLAG_GCIN_client_handle_has_focus = 1,
  FLAG_GCIN_client_handle_use_preedit = 2,
  FLAG_GCIN_client_handle_raise_window = 0x1000  // for mozilla, dirty fix
};

enum {
  FLAG_GCIN_srv_ret_status_use_pop_up = 1    // If this is used, we don't need the dirty fix
};


#ifdef __cplusplus
extern "C" {
#endif

GCIN_client_handle *gcin_im_client_open(Display *disp);
void gcin_im_client_close(GCIN_client_handle *handle);
void gcin_im_client_focus_in(GCIN_client_handle *handle);
void gcin_im_client_focus_out(GCIN_client_handle *handle);
void gcin_im_client_focus_out2(GCIN_client_handle *handle, char **rstr);
void gcin_im_client_set_window(GCIN_client_handle *handle, Window win);
void gcin_im_client_set_cursor_location(GCIN_client_handle *handle,
                                        int x, int y);
/*  rstr returns UTF-8 encoded string, you should use 'free()' to free the
    memory.

    return boolean:
      FALSE : the key is rejected, should use client's own result(ASCII key).
      TRUE : the key is accepted, translated result is in rstr.
 */
int gcin_im_client_forward_key_press(GCIN_client_handle *handle,
                                          KeySym key, u_int state,
                                          char **rstr);
int gcin_im_client_forward_key_release(GCIN_client_handle *handle,
                                          KeySym key, u_int state,
                                          char **rstr);
#if WIN32
bool gcin_im_client_key_eaten(GCIN_client_handle *handle, int press_release,
                                          KeySym key, u_int state);
#endif

void gcin_im_client_set_flags(GCIN_client_handle *handle, int flags, int *ret_flags);
void gcin_im_client_clear_flags(GCIN_client_handle *handle, int flags, int *ret_flags);

void gcin_im_client_reset(GCIN_client_handle *handle);
void gcin_im_client_message(GCIN_client_handle *handle, char *message);

#include "gcin-im-client-attr.h"
int gcin_im_client_get_preedit(GCIN_client_handle *handle, char **str, GCIN_PREEDIT_ATTR att[], int *cursor, int *sub_comp_len);

#if UNIX
Window find_gcin_window(Display *dpy);
#else
HWND find_gcin_window();
void gcin_im_client_set_tsin_pho_mode(GCIN_client_handle *handle, int pho_mode);
#endif


#ifdef __cplusplus
}
#endif


#endif
