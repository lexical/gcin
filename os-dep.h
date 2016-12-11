#ifndef OS_DEP_H
#define OS_DEP_H
#if WIN32
#include <windows.h>
#include "win32-key.h"
//typedef HWND Window;
typedef unsigned int Window; // HWND is actual a pointer, this breaks 64-bit IME & 32-bit gcin
typedef void Display;
typedef unsigned int u_int;
typedef u_int KeySym;
typedef unsigned char u_char;
typedef unsigned int CARD32;
typedef __int64 u_int64_t;
bool win32exec(char *s);
int win32exec_script(char *s, char *para=NULL);
int win32exec_script_va(char *s, ...);
int win32exec_va(char *s, ...);
bool win32exec_para(char *s, char *para);
extern char *gcin_program_files_path;
extern char *gcin_script_path;
typedef struct {
	int x, y;
} XPoint;
#define snprintf sprintf_s
#define bzero ZeroMemory
#define true 1
#define True 1
#define false 0
int utf8_to_16(char *text, wchar_t *wtext, int wlen);
int utf16_to_8(wchar_t *in, char *out, int outN);
inline void *GDK_DISPLAY() { return NULL;}
//typedef wchar_t unich_t;
#define _L(x)      L ## x
#pragma execution_character_set("utf-8")
#else
typedef char unich_t;
void unix_exec(char *fmt,...);
#define _L(x) x
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/keysym.h>
#include <glib.h>
#if GLIB_CHECK_VERSION(2,29,8)
#define G_CONST_RETURN const
#endif
#include <gdk/gdkx.h>
#endif
#endif
