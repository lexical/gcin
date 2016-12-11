#include "gcin.h"
#if UNIX
#include <errno.h>
#endif

#if UNIX
#if !CLIENT_LIB || DEBUG
static FILE *out_fp;
#endif

void p_err(char *fmt,...)
{
  va_list args;
  char out[4096];

  va_start(args, fmt);
  vsprintf(out, fmt, args);
  va_end(args);

#if CLIENT_LIB
  fprintf(stderr, "%s\n", out);
#else
  if (getenv("NO_GTK_INIT"))
    fprintf(stderr, "%s\n", out);
  else {
    GtkWidget *dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "%s", out);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  }
#endif

#if DEBUG && 1
  abort();
#else
  if (getenv("GCIN_ERR_COREDUMP"))
    abort();
  exit(-1);
#endif
}

void p_err_no_alert(char *fmt,...)
{
  va_list args;
  char out[4096];

  va_start(args, fmt);
  vsprintf(out, fmt, args);
  va_end(args);

  fprintf(stderr, "%s\n", out);

#if DEBUG && 1
  abort();
#else
  if (getenv("GCIN_ERR_COREDUMP"))
    abort();
  exit(-1);
#endif
}

#if !CLIENT_LIB || DEBUG
static void init_out_fp()
{
  if (!out_fp) {
    if (getenv("GCIN_DBG_TMP") || 0) {
      char fname[64];
      sprintf(fname, "/tmp/gcindbg-%d-%d", getuid(), getpid());
      out_fp = fopen(fname, "w");
    }

    if (!out_fp)
      out_fp = stdout;
  }
}
#endif

#if !CLIENT_LIB
void dbg_time(char *fmt,...)
{
  va_list args;
  time_t t;

  init_out_fp();

  time(&t);
  struct tm *ltime = localtime(&t);
  dbg("%02d:%02d:%02d ", ltime->tm_hour, ltime->tm_min, ltime->tm_sec);

  va_start(args, fmt);
  vfprintf(out_fp, fmt, args);
  fflush(out_fp);
  va_end(args);
}
#endif

#if DEBUG
void __gcin_dbg_(const char *fmt,...)
{
  va_list args;

  init_out_fp();

  va_start(args, fmt);
  vfprintf(out_fp, fmt, args);
  fflush(out_fp);
  va_end(args);
}
#endif

char *sys_err_strA()
{
  return (char *)strerror(errno);
}

#else
#include <share.h>
#include <io.h>
#include <strsafe.h>

#if _DEBUG
#define _DBG 1
#define CONSOLE_OFF 0
#endif


#if _DBG
FILE *dbgfp;
#endif
#if GCIN_SVR
void dbg_time(char *fmt,...)
{
}
#endif

char utf8_sigature[];
static void init_dbgfp()
{
#if _DBG
	if (!dbgfp) {
#if (!GCIN_IME || 1) && !CONSOLE_OFF
		AllocConsole();
		fclose(stdout);
		fclose(stderr);
		int fh = _open_osfhandle((long)GetStdHandle(STD_OUTPUT_HANDLE), 0);
		_dup2(fh, 1);
		_dup2(fh, 2);
		_fdopen(1, "wt");
		_fdopen(2, "wt");
		fflush(stdout);
#endif
		char tt[512];
#if GCIN_IME
		sprintf(tt, "C:\\dbg\\ime%x", GetCurrentProcessId());
#elif GCIN_SVR
#if 0
		sprintf(tt, "C:\\dbg\\svr%x", GetCurrentProcessId());
#else
		sprintf(tt, "C:\\dbg\\gcin");
#endif
#else
		sprintf(tt, "C:\\dbg\\other", GetCurrentProcessId());
#endif
		dbgfp=_fsopen(tt, "wt",  _SH_DENYWR);
		setbuf(dbgfp, NULL);
		fprintf(dbgfp, "%s", utf8_sigature);

		char exe[MAX_PATH];
		GetModuleFileNameA(NULL, exe, sizeof(exe));
		dbg("started %s\n", exe);
	}
#endif
}

int utf8_to_big5(char *in, char *out, int outN);

#if DEBUG
void __gcin_dbg_(const char *format, ...) {
#if _DBG
	va_list ap;
	va_start(ap, format);

	init_dbgfp();

	char buf[1024];

	vsprintf_s(buf, sizeof(buf), format, ap);
	char bufb5[1024];
#if 1
	utf8_to_big5(buf, bufb5, sizeof(bufb5));
#else
	strcpy(bufb5, buf);
#endif

	fprintf(dbgfp, "%s", bufb5);
	printf("%s", bufb5);
	wchar_t wchstr[1024];
	utf8_to_16(buf, wchstr, ARRAYSIZE(wchstr));
	OutputDebugStringW(wchstr);

	fflush(dbgfp);
	va_end(ap);
#endif
}
#endif

char *err_strA(DWORD dw)
{
	WCHAR msgstr[128];
	static char msg8[256];
    LPVOID lpMsgBuf;

    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    StringCchPrintfW(msgstr, ARRAYSIZE(msgstr), L"%d: %s", dw, lpMsgBuf);

	utf16_to_8(msgstr, msg8, sizeof(msg8));
	return msg8;
}


char *sys_err_strA()
{
	return err_strA(GetLastError());
}

void p_err(char *format, ...) {

	va_list ap;
	va_start(ap, format);
#if _DBG
	init_dbgfp();
	vfprintf_s(dbgfp, format, ap);
	vprintf(format, ap);
	fflush(dbgfp);
#endif
	char tt[512];
	vsprintf_s(tt, sizeof(tt), format, ap);
	WCHAR exe[512];
	GetModuleFileNameW(NULL, exe, ARRAYSIZE(exe));

	WCHAR u16[512];
	utf8_to_16(tt, u16, ARRAYSIZE(u16));
	MessageBoxW(NULL, u16, exe, MB_OK);
	exit(0);
	va_end(ap);

}

#if GCIN_SVR
void ErrorExit(LPTSTR lpszFunction)
{
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"),
        lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}
#endif
#endif


void *zmalloc(int n)
{
  void *p =  malloc(n);
  bzero(p, n);
  return p;
}
#if !GCIN_IME

void *memdup(void *p, int n)
{
  if (!p || !n)
    return NULL;
  void *q;
  q = malloc(n);
  memcpy(q, p, n);
  return q;
}

// can handle eol with \n \r \n\r \r\n
char *myfgets(char *buf, int bufN, FILE *fp)
{
	char *out = buf;
//	int rN = 0;
	while (!feof(fp) && out - buf < bufN) {
		char a, b;
		a = 0;
		if (fread(&a, 1, 1, fp) != 1)
			break;
		if (a =='\n') {
			b = 0;
			if (fread(&b, 1, 1, fp)==1)
				if (b!='\r')
					fseek(fp, -1, SEEK_CUR);
			break;
		} else
		if (a =='\r') {
			b = 0;
			if (fread(&b, 1, 1, fp)==1)
				if (b!='\n')
					fseek(fp, -1, SEEK_CUR);
			break;
		}

		*(out++) = a;
	}

	*out = 0;
	return buf;
}
#endif

#if GCIN_SVR
#if WIN32
#include <gdk/gdkwin32.h>
void win32_init_win(GtkWidget *win)
{
  HWND handle=(HWND)gdk_win32_drawable_get_handle(win->window);

  ShowWindow(handle, SW_HIDE);

  SetWindowLong(handle, GWL_EXSTYLE, WS_EX_NOACTIVATE|WS_EX_TOPMOST);
  SetWindowPos(handle, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
//  ShowWindow(handle, SW_SHOW);
}
#endif
#endif

#if !GCIN_IME && !CLIENT_LIB && !GCIN_EXIT
void box_warn(char *fmt,...)
{
  va_list args;
  char out[4096];

  va_start(args, fmt);
  vsprintf(out, fmt, args);
  va_end(args);

  GtkWidget *dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_ERROR,
                                   GTK_BUTTONS_CLOSE,
                                    "%s", out);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}
#endif
