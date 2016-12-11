#include <stdio.h>
#include <stdlib.h>
#include "os-dep.h"
#include "gcin.h"

#if UNIX
static void exec_script(char *name)
{
  char scr[512];

  sprintf(scr, GCIN_SCRIPT_DIR"/%s", name);
  system(scr);
}
#endif

void exec_setup_scripts()
{
#if WIN32
win32exec_script("gcin-user-setup.bat");

char gcin_table[128];
strcat(strcpy(gcin_table, getenv("GCIN_DIR")), "\\table");
char *app_gcin=getenv("APPDATA_GCIN");

char *files[]={
"pho.tab2", "pho-huge.tab2",
"s-pho.tab2", "s-pho-huge.tab2",
"tsin32", "tsin32.idx",
"en-american", "en-american.idx",
"s-tsin32", "s-tsin32.idx",
"symbol-table", "phrase.table"};
for(int i=0; i < sizeof(files)/sizeof(files[0]); i++) {
	char src[MAX_PATH], dest[MAX_PATH];
	sprintf(src, "%s\\%s", gcin_table, files[i]);
	sprintf(dest, "%s\\%s", app_gcin, files[i]);

//	dbg("%s -> %s\n", src, dest);
	CopyFileA(src, dest, true);
}
#else
  exec_script("gcin-user-setup "GCIN_TABLE_DIR" "GCIN_BIN_DIR);
#endif
}
