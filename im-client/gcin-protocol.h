#if UNIX
#include <X11/Xlib.h>
#endif
#include "../gcin-endian.h"

typedef enum {
  GCIN_req_key_press = 1,
  GCIN_req_key_release = 2,
  GCIN_req_focus_in = 4,
  GCIN_req_focus_out = 8,
  GCIN_req_set_cursor_location = 0x10,
  GCIN_req_set_flags = 0x20,
  GCIN_req_get_preedit = 0x40,
  GCIN_req_reset = 0x80,
  GCIN_req_focus_out2 = 0x100,
  GCIN_req_message = 0x200,
  GCIN_req_test_key_press = 0x400,
  GCIN_req_test_key_release = 0x800,
  GCIN_req_set_tsin_pho_mode = 0x1000,
} GCIN_req_t;


typedef struct {
#if 0
    KeySym key;
#else
    u_int key;
#endif
    u_int state;
} KeyEvent;

typedef struct {
    short x, y;
} GCINpoint;


typedef struct {
  u_int req_no;  // to make the im server stateless, more is better
  u_int client_win;
  u_int flag;
  u_int input_style;
  GCINpoint spot_location;

  union {
    KeyEvent keyeve;
    char dummy[32];   // for future expansion
  };
} GCIN_req;


enum {
  GCIN_reply_key_processed = 1,
  GCIN_reply_key_state_disabled = 2,
};


typedef struct {
  u_int flag;
  u_int datalen;    // '\0' shoule be counted if data is string
} GCIN_reply;


#define __GCIN_PASSWD_N_ (31)

#if !WIN32
typedef struct GCIN_PASSWD {
  u_int seed;
  u_char passwd[__GCIN_PASSWD_N_];
} GCIN_PASSWD;
#endif

typedef struct {
  u_int ip;
  u_short port;
#if !WIN32
  GCIN_PASSWD passwd;
#endif
} Server_IP_port;

typedef struct {
  char sock_path[80];
} Server_sock_path;
#if UNIX
void __gcin_enc_mem(u_char *p, int n, GCIN_PASSWD *passwd, u_int *seed);
#endif

#define SHARED_MEMORY 0

#if WIN32
#define GCIN_WIN_NAME "gcin0"
#define GCIN_PORT_MESSAGE WM_USER+10
#define GCIN_CLIENT_MESSAGE_REQ WM_USER+11
#define GCIN_CLIENT_MESSAGE_CLOSE WM_USER+12
#if SHARED_MEMORY
#define SHARED_MEM_NAME "Global\\gcin_IME_IPC%x_%x"
typedef struct GCIN_SHM_S {
	HANDLE handle;
	void *buf;
	int read_index;
	int port;
} *GCIN_SHM;

GCIN_SHM gcin_create_shm(HWND hwnd, int port);
GCIN_SHM gcin_open_shm(HWND hwnd, int port);
void gcin_start_shm_read(GCIN_SHM sh);
int gcin_shm_read(GCIN_SHM sh, void *buf, int readN);
void gcin_start_shm_write(GCIN_SHM sh);
int gcin_shm_write(GCIN_SHM sh, void *buf, int writeN);
void gcin_shm_close(GCIN_SHM sh);


#else
#define GCIN_PIPE_PATH "\\\\.\\pipe\\gcin-svr%d"
#endif
#endif