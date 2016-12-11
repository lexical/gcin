#if UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <string.h>
#include "gcin.h"
#include "gcin-protocol.h"
#include "gcin-im-client.h"
#include "im-srv.h"
#include <gtk/gtk.h>
#include "util.h"

#define DBG 0

#if UNIX
static int myread(int fd, void *buf, int bufN)
#else
#if SHARED_MEMORY
static int myread(GCIN_SHM fd, void *buf, int bufN)
#else
static int myread(HANDLE fd, void *buf, int bufN)
#endif
#endif
{
  int ofs=0, toN = bufN;

#if UNIX
  while (toN) {
    fd_set rfds;
    struct timeval tv;
    int retval;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    retval = select(fd+1, &rfds, NULL, NULL, &tv);

    if (retval <= 0) {
      dbg("select error\n");
      return -1;
    }

    int rn;
    if ((rn=read(fd, ((char *)buf) + ofs, toN)) < 0) {
      dbg("read error");
      return -1;
    }

    if (rn==0)
      break;

    ofs+=rn;
    toN-=rn;
  };
  return ofs;
#else
#if SHARED_MEMORY
  return gcin_shm_read(fd, buf, bufN);
#else
  while (toN) {
    DWORD bytes = 0;
     for(int loop=0;loop < 10000; loop++) {
       bytes = 0;
       if (PeekNamedPipe(fd, NULL, 0, NULL, &bytes, NULL)) {
//         dbg("bytes %d\n", bytes);
       } else
         dbg("PeekNamedPipe failed %s", sys_err_strA());

       if (bytes > 0)
         break;

       Sleep(10);
     }

     if (!bytes)
       return -1;

	dbg("bytes:%d %d\n", bytes, toN);

	if (bytes > toN)
		bytes = toN;

    DWORD rn;
    BOOL r = ReadFile(fd, ((char *)buf) + ofs, bytes, &rn, 0);
    if (!r)
      return -1;
    ofs+=rn;
    toN-=rn;
  };
  return bufN;
#endif
#endif
}


GCIN_ENT *gcin_clients;
int gcin_clientsN;

#if WIN32
// need to use Enter/Leave CriticalSection in the future
int find_im_client(HANDLE hand)
{
	int i;
	for(i=0;i<gcin_clientsN;i++)
		if (gcin_clients[i].fd == hand)
			break;
	if (i==gcin_clientsN)
		return -1;
	return i;
}

#if SHARED_MEMORY
void add_im_client(GCIN_SHM hand, int i, DWORD pid)
{
	if (i==gcin_clientsN) {
		gcin_clientsN++;
		gcin_clients=trealloc(gcin_clients, GCIN_ENT, gcin_clientsN);
	}

	ZeroMemory(&gcin_clients[i], sizeof(GCIN_ENT));
	gcin_clients[i].fd = hand;
	gcin_clients[i].pid = pid;
}
#else
int add_im_client(HANDLE hand)
{
	int i = find_im_client(0);
	if (i<0) {
		gcin_clients=trealloc(gcin_clients, GCIN_ENT, gcin_clientsN);
		i=gcin_clientsN++;
	}

	ZeroMemory(&gcin_clients[i], sizeof(GCIN_ENT));
	gcin_clients[i].fd = hand;
	return i;
}
#endif
#endif

extern GCIN_PASSWD my_passwd;

gboolean ProcessKeyPress(KeySym keysym, u_int kev_state);
gboolean ProcessTestKeyPress(KeySym keysym, u_int kev_state);
gboolean ProcessKeyRelease(KeySym keysym, u_int kev_state);
gboolean ProcessTestKeyRelease(KeySym keysym, u_int kev_state);
int gcin_FocusIn(ClientState *cs);
int gcin_FocusOut(ClientState *cs);
void update_in_win_pos();
void hide_in_win(ClientState *cs);
void init_state_chinese(ClientState *cs, gboolean tsin_pho_mode);
void clear_output_buffer();
void flush_edit_buffer();
int gcin_get_preedit(ClientState *cs, char *str, GCIN_PREEDIT_ATTR attr[], int *cursor, int *sub_comp_len);
void gcin_reset();
void dbg_time(char *fmt,...);

extern char *output_buffer;
extern int output_bufferN;

#if UNIX
int write_enc(int fd, void *p, int n)
#else
#if SHARED_MEMORY
int write_enc(GCIN_SHM fd, void *p, int n)
#else
int write_enc(HANDLE fd, void *p, int n)
#endif
#endif
{
#if WIN32
#if SHARED_MEMORY
  return gcin_shm_write(fd, p, n);
#else
  int loop=0;
  int twN=0;
  while (n > 0 && loop < 50) {
    DWORD wn;
     BOOL r = WriteFile(fd, (char *)p, n, &wn, 0);
     if (!r) {
       dbg("write_enc %s\n", sys_err_strA());
	   return -1;
     }

	 twN+=wn;
	 n-=wn;
	 loop++;
	 p=(char *)p+wn;
  }

  return twN;
#endif
#else
  if (!fd)
    return 0;

  unsigned char *tmp = (unsigned char *)malloc(n);
  memcpy(tmp, p, n);
  if (gcin_clients[fd].type == Connection_type_tcp) {
    __gcin_enc_mem(tmp, n, &srv_ip_port.passwd, &gcin_clients[fd].seed);
  }
  int r =  write(fd, tmp, n);

#if DBG
  if (r < 0)
    perror("write_enc");
#endif

  free(tmp);

  return r;
#endif
}

#ifdef __cplusplus
extern "C" void gdk_input_remove	  (gint		     tag);
#endif

#if WIN32
typedef int socklen_t;
#else
#include <pwd.h>
#endif

#if UNIX
static void shutdown_client(int fd)
#else
#if SHARED_MEMORY
void shutdown_client(GCIN_SHM fd)
#else
static void shutdown_client(HANDLE fd)
#endif
#endif
{
  dbg("client shutdown fd %d\n", fd);
#if UNIX
  g_source_remove(gcin_clients[fd].tag);
  int idx = fd;
#else
  int idx = find_im_client(fd);
#endif

  if (gcin_clients[idx].cs == current_CS) {
    hide_in_win(current_CS);
    current_CS = NULL;
  }

dbg("llllll\n");

  free(gcin_clients[idx].cs);
  gcin_clients[idx].cs = NULL;
#if UNIX
  g_io_channel_unref(gcin_clients[idx].channel);
  dbg("after g_object_unref\n");
  gcin_clients[idx].fd = 0;
#else
  gcin_clients[idx].fd = NULL;
#endif

#if UNIX && 0
  int uid = getuid();
  struct passwd *pwd;
  if (uid>0 && uid<500)
    exit(0);
#endif
#if UNIX
  close(fd);
#else
#if SHARED_MEMORY
  gcin_shm_close(fd);
#else
  CloseHandle(fd);
#endif
//  CloseHandle(handle);
#endif
}

void message_cb(char *message);
void save_CS_temp_to_current();
void disp_tray_icon();
void gcin_set_tsin_pho_mode(ClientState *cs, gboolean pho_mode);
#if WIN32
extern int dpy_x_ofs, dpy_y_ofs;
#endif

#if UNIX
void process_client_req(int fd)
#else
#if SHARED_MEMORY
void process_client_req(GCIN_SHM fd)
#else
void process_client_req(HANDLE fd)
#endif
#endif
{
  GCIN_req req;
#if DBG
  dbg("svr--> process_client_req %d\n", fd);
#endif

#if SHARED_MEMORY
  gcin_start_shm_read(fd);
#endif
  int rn = myread(fd, &req, sizeof(req));

  if (rn <= 0) {
    shutdown_client(fd);
    return;
  }
#if UNIX
  if (gcin_clients[fd].type == Connection_type_tcp) {
    __gcin_enc_mem((u_char *)&req, sizeof(req), &srv_ip_port.passwd, &gcin_clients[fd].seed);
  }
#endif
  to_gcin_endian_4(&req.req_no);
  to_gcin_endian_4(&req.client_win);
  to_gcin_endian_4(&req.flag);
  to_gcin_endian_2(&req.spot_location.x);
  to_gcin_endian_2(&req.spot_location.y);

//  dbg("spot %d %d\n", req.spot_location.x, req.spot_location.y);

  ClientState *cs = NULL;

  if (current_CS && req.client_win == current_CS->client_win) {
    cs = current_CS;
  } else {
#if UNIX
    int idx = fd;
    cs = gcin_clients[fd].cs;
#else
    int idx = find_im_client(fd);
    cs = gcin_clients[idx].cs;
#endif

    int new_cli = 0;
    if (!cs) {
      cs = gcin_clients[idx].cs = tzmalloc(ClientState, 1);
      new_cli = 1;
    }

    cs->client_win = req.client_win;
    cs->b_gcin_protocol = TRUE;
    cs->input_style = InputStyleOverSpot;

#if WIN32
    cs->use_preedit = TRUE;
#endif

#if UNIX
    if (gcin_init_im_enabled && new_cli)
#else
    if (new_cli)
#endif
    {
      dbg("new_cli default_input_method:%d cs:%x\n", default_input_method, cs);
#if UNIX
      if (!current_CS)
#endif
      {
        current_CS = cs;
        save_CS_temp_to_current();
      }

      init_state_chinese(cs, ini_tsin_pho_mode);
      disp_tray_icon();
    }
  }

  if (!cs)
    p_err("bad cs\n");

  if (req.req_no != GCIN_req_message) {
#if UNIX
    cs->spot_location.x = req.spot_location.x;
    cs->spot_location.y = req.spot_location.y;
#else
    cs->spot_location.x = req.spot_location.x - dpy_x_ofs;
    cs->spot_location.y = req.spot_location.y - dpy_y_ofs;

	dbg("req.spot_location.x %d %d\n", req.spot_location.x, dpy_x_ofs);
#endif
  }

  gboolean status;
  GCIN_reply reply;
  bzero(&reply, sizeof(reply));

  switch (req.req_no) {
    case GCIN_req_key_press:
    case GCIN_req_key_release:
      current_CS = cs;
      save_CS_temp_to_current();

#if DBG && 0
      {
        char tt[128];

        if (req.keyeve.key < 127) {
          sprintf(tt,"'%c'", req.keyeve.key);
        } else {
          strcpy(tt, XKeysymToString(req.keyeve.key));
        }

        dbg_time("GCIN_key_press  %x %s\n", cs, tt);
      }
#endif
      to_gcin_endian_4(&req.keyeve.key);
      to_gcin_endian_4(&req.keyeve.state);

//	  dbg("serv key eve %x %x predit:%d\n",req.keyeve.key, req.keyeve.state, cs->use_preedit);

#if DBG
	  char *typ;
      typ="press";
#endif
#if 0
      if (req.req_no==GCIN_req_key_press)
        status = Process2KeyPress(req.keyeve.key, req.keyeve.state);
      else {
        status = Process2KeyRelease(req.keyeve.key, req.keyeve.state);
#else
      if (req.req_no==GCIN_req_key_press)
        status = ProcessKeyPress(req.keyeve.key, req.keyeve.state);
      else {
        status = ProcessKeyRelease(req.keyeve.key, req.keyeve.state);
#endif

#if DBG
        typ="rele";
#endif
      }

      if (status)
        reply.flag |= GCIN_reply_key_processed;
#if DBG
      dbg("%s srv flag:%x status:%d len:%d %x %c\n",typ, reply.flag, status, output_bufferN, req.keyeve.key,req.keyeve.key & 0x7f);
#endif
      int datalen;
      datalen = reply.datalen =
        output_bufferN ? output_bufferN + 1 : 0; // include '\0'
      to_gcin_endian_4(&reply.flag);
      to_gcin_endian_4(&reply.datalen);

#if SHARED_MEMORY
	  gcin_start_shm_write(fd);
#endif
      write_enc(fd, &reply, sizeof(reply));

//      dbg("server reply.flag %x\n", reply.flag);

      if (output_bufferN) {
        write_enc(fd, output_buffer, datalen);
        clear_output_buffer();
      }

      break;
#if WIN32
    case GCIN_req_test_key_press:
    case GCIN_req_test_key_release:
      current_CS = cs;
      save_CS_temp_to_current();
      to_gcin_endian_4(&req.keyeve.key);
      to_gcin_endian_4(&req.keyeve.state);

#if   DBG
	  dbg("%s %x %x predit:%d\n", req.req_no == GCIN_req_test_key_press?"key_press":"key_release",
	  req.keyeve.key, req.keyeve.state, cs->use_preedit);
#endif

      if (req.req_no==GCIN_req_test_key_press)
        status = ProcessTestKeyPress(req.keyeve.key, req.keyeve.state);
      else
        status = ProcessTestKeyRelease(req.keyeve.key, req.keyeve.state);

      if (status)
        reply.flag |= GCIN_reply_key_processed;

      reply.datalen = 0;
      to_gcin_endian_4(&reply.flag);
      to_gcin_endian_4(&reply.datalen);

#if SHARED_MEMORY
	  gcin_start_shm_write(fd);
#endif
      write_enc(fd, &reply, sizeof(reply));
      break;
#endif
    case GCIN_req_focus_in:
#if DBG
      dbg_time("GCIN_req_focus_in  %x %d %d\n",cs, cs->spot_location.x, cs->spot_location.y);
#endif
#if 1
      current_CS = cs;
#endif
      gcin_FocusIn(cs);
      break;
    case GCIN_req_focus_out:
#if DBG
      dbg_time("GCIN_req_focus_out  %x\n", cs);
#endif
      gcin_FocusOut(cs);
      break;
#if UNIX
    case GCIN_req_focus_out2:
      {
#if DBG
      dbg_time("GCIN_req_focus_out2  %x\n", cs);
#endif
      if (gcin_FocusOut(cs))
        flush_edit_buffer();

      GCIN_reply reply;
      bzero(&reply, sizeof(reply));

      int datalen = reply.datalen =
        output_bufferN ? output_bufferN + 1 : 0; // include '\0'
      to_gcin_endian_4(&reply.flag);
      to_gcin_endian_4(&reply.datalen);
      write_enc(fd, &reply, sizeof(reply));

//      dbg("server reply.flag %x\n", reply.flag);

      if (output_bufferN) {
        write_enc(fd, output_buffer, datalen);
        clear_output_buffer();
      }
      }
      break;
#endif
    case GCIN_req_set_cursor_location:
#if DBG || 0
      dbg_time("set_cursor_location %p %d %d\n", cs,
         cs->spot_location.x, cs->spot_location.y);
#endif
      update_in_win_pos();
      break;
    case GCIN_req_set_flags:
#if DBG
      dbg("GCIN_req_set_flags\n");
#endif
      if (BITON(req.flag, FLAG_GCIN_client_handle_raise_window)) {
#if DBG
        dbg("********* raise * window\n");
#endif
        if (!gcin_pop_up_win)
          cs->b_raise_window = TRUE;
      }

	  if (req.flag & FLAG_GCIN_client_handle_use_preedit)
        cs->use_preedit = TRUE;

      int rflags;
      rflags = 0;
      if (gcin_pop_up_win)
        rflags = FLAG_GCIN_srv_ret_status_use_pop_up;
#if SHARED_MEMORY
	  gcin_start_shm_write(fd);
#endif
      write_enc(fd, &rflags, sizeof(rflags));
      break;
    case GCIN_req_get_preedit:
      {
#if DBG
      dbg("svr GCIN_req_get_preedit %x\n", cs);
#endif
      char str[GCIN_PREEDIT_MAX_STR];
      GCIN_PREEDIT_ATTR attr[GCIN_PREEDIT_ATTR_MAX_N];
      int cursor, sub_comp_len;
      int attrN = gcin_get_preedit(cs, str, attr, &cursor, &sub_comp_len);
      if (gcin_edit_display&(GCIN_EDIT_DISPLAY_BOTH|GCIN_EDIT_DISPLAY_OVER_THE_SPOT))
        cursor=0;
      if (gcin_edit_display&GCIN_EDIT_DISPLAY_OVER_THE_SPOT) {
        attrN=0;
        str[0]=0;
      }

#if SHARED_MEMORY
	  gcin_start_shm_write(fd);
#endif
      int len = strlen(str)+1; // including \0
      if (write_enc(fd, &len, sizeof(len)) < 0)
		  break;
      if (write_enc(fd, str, len) < 0)
		  break;
//      dbg("attrN:%d\n", attrN);
      if (write_enc(fd, &attrN, sizeof(attrN)) < 0)
		  break;
      if (attrN > 0) {
        if (write_enc(fd, attr, sizeof(GCIN_PREEDIT_ATTR)*attrN) < 0)
			break;
	  }
      if (write_enc(fd, &cursor, sizeof(cursor)) < 0)
		  break;
#if WIN32 || 1
      if (write_enc(fd, &sub_comp_len, sizeof(sub_comp_len)) < 0)
		  break;
#endif
//      dbg("uuuuuuuuuuuuuuuuu len:%d %d cursor:%d\n", len, attrN, cursor);
      }
      break;
    case GCIN_req_reset:
#if DBG
      dbg("GCIN_req_reset\n");
#endif
      gcin_reset();
      break;
    case GCIN_req_message:
      {
//        dbg("GCIN_req_message\n");
        short len=0;
        int rn = myread(fd, &len, sizeof(len));
        if (rn <= 0) {
cli_down:
          shutdown_client(fd);
          return;
        }

        // only unix socket, no decrypt
        char buf[512];
        // message should include '\0'
        if (len > 0 && len < sizeof(buf)) {
          if (myread(fd, buf, len)<=0)
            goto cli_down;
          message_cb(buf);
        }
      }
      break;
#if WIN32
	case GCIN_req_set_tsin_pho_mode:
	  gcin_set_tsin_pho_mode(cs, req.flag);
	  break;
#endif
    default:
      dbg_time("Invalid request %x from:", req.req_no);
#if UNIX
      struct sockaddr_in addr;
      socklen_t len=sizeof(addr);
      bzero(&addr, sizeof(addr));

      if (!getpeername(fd, (struct sockaddr *)&addr, &len)) {
        dbg("%s\n", inet_ntoa(addr.sin_addr));
      } else {
        perror("getpeername\n");
      }
#endif
      shutdown_client(fd);
      break;
  }
}
