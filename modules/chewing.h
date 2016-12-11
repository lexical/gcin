#ifndef CHEWING_H
#define CHEWING_H

#include <chewing/chewing.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "gcin.h"
#include "pho.h"
#include "gst.h"
#include "im-client/gcin-im-client-attr.h"
#include "win1.h"
#include "gcin-module.h"
#include "gcin-module-cb.h"
#include "gcin-conf.h"
#include "pho-kbm-name.h"

#define MAX_SEG_NUM  128
#define GCIN_CHEWING_CONFIG "/.gcin/config/chewing_conf.dat"
#define GCIN_KB_CONFIG "/.gcin/config/phonetic-keyboard2"

typedef struct _SEGMENT {
    GtkWidget *label;
    unsigned char selidx, selN;
} SEG;

typedef struct KB_MAPPING
{
    char *pszGcinKbName;
    char *pszChewingKbName;
} kbmapping_t;

// gcin module callbacks
int module_init_win (GCIN_module_main_functions *pFuncs);
void module_get_win_geom (void);
int module_reset (void);
int module_get_preedit (char *pszStr, GCIN_PREEDIT_ATTR attr[],
                        int *pnCursor, int *pCompFlag);
gboolean module_feedkey (int nKeyVal, int nKeyState);
int module_feedkey_release (KeySym xkey, int nKbState);
void module_move_win(int x, int y);
void module_change_font_size (void);
void module_show_win (void);
void module_hide_win (void);
int module_win_visible (void);
int module_flush_input (void);

// config funcs
void chewing_config_open (gboolean bWrite);
void chewing_config_load (ChewingConfigData *pChewingConfig);
void chewing_config_set (ChewingContext *pChewingContext);
void chewing_config_dump (void);
void chewing_config_close (void);
gboolean chewing_config_save (int nVal[]);

#endif
