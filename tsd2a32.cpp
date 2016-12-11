// gcin

#include "gcin.h"
#include "pho.h"
#include "tsin.h"

int phcount;
void prph2(FILE *fp, phokey_t kk);

#if WIN32
void init_gcin_program_files();
 #pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#endif

void get_keymap_str(u_int64_t k, char *keymap, int keybits, char tkey[]);
char *phokey2pinyin(phokey_t k);
gboolean is_pinyin_kbm();
char *sys_err_strA();
void init_TableDir();
extern char *tsin32_f;
gboolean load_tsin_db_ex(TSIN_HANDLE *ptsin_hand, char *infname, gboolean read_only, gboolean use_idx);

void swap_ptr(char **a, char **b)
{
  char *t;
  t = *a;
  *a = *b;
  *b = t;
}

void set_is_chs();
gboolean tsin_seek_ex(TSIN_HANDLE *ptsin_hand, void *pho, int plen, int *r_sti, int *r_edi, char *tone_mask);
void load_tsin_entry_ex(TSIN_HANDLE *ptsin_hand, int idx, char *len, usecount_t *usecount, void *pho, u_char *ch);

int main(int argc, char **argv)
{
  FILE *fp;
  int i;
  char clen;
  usecount_t usecount;
  gboolean pr_usecount = TRUE;
#define MAX_MINUS_FILES 16
  char *fnames_minus[MAX_MINUS_FILES];
  int fnames_minusN = 0;
  gboolean binary_out = FALSE;
  char *fname = NULL;
  char *fname_out = NULL;
  char *fname_out_save = NULL;
  char fname_tmp[128];
  char ts_user_fname[128];
  gboolean no_pho = FALSE;

  gtk_init(&argc, &argv);

  set_is_chs();
  load_setttings();

  init_TableDir();

  gboolean b_pinyin = is_pinyin_kbm();

  for(i=1; i < argc;) {
    if (!strcmp(argv[i], "-nousecount")) {
      i++;
      pr_usecount = FALSE;
      b_pinyin = FALSE;
    } else
    if (!strcmp(argv[i], "-minus")) {
      if (i==argc-1 || argv[i+1][0]=='-')
        p_err("-o need minus file name");
      fnames_minus[fnames_minusN++] = argv[i+1];
      i+=2;
    } else
    if (!strcmp(argv[i], "-b")) {
      i++;
      binary_out = TRUE;
    } else
    if (!strcmp(argv[i], "-np")) {
      i++;
      no_pho = TRUE;
    } else
    if (!strcmp(argv[i], "-o")) {
      if (i==argc-1 || argv[i+1][0]=='-')
        p_err("-o need out file name");
        fname_out = argv[i+1];
        i+=2;
    } else
      fname = argv[i++];
  }

  for(i=0;i<fnames_minusN;i++)
    dbg(" %s\n", fnames_minus[i]);

  TSIN_HANDLE tsin_hands[MAX_MINUS_FILES];

  bzero(&tsin_hands, sizeof(tsin_hands));

#if 0
  fnames_minusN = 1;
#endif

  if (fnames_minusN) {
    set_is_chs();

    int nfnames_minusN=0;
    for(i=0;i<fnames_minusN;i++) {
      dbg("fnames_minus %d] %s\n", i, fnames_minus[i]);

      TSIN_HANDLE t;
      bzero(&t, sizeof(t));
      if (load_tsin_db_ex(&t, fnames_minus[i], TRUE, TRUE))
    	  tsin_hands[nfnames_minusN++] = t;
    }
    fnames_minusN = nfnames_minusN;

    if (!fname) {
      get_gcin_user_fname(tsin32_f, ts_user_fname);
      fname = ts_user_fname;
    }
  }

#if 0
  fnames_minusN = 1;
#endif

  if (!fname)
    p_err("%s: tsin32 file name expected\n", argv[0]);


  if (binary_out) {
    get_gcin_user_fname("tsd2a_tmp", fname_tmp);
    fname_out_save = fname_out;
    fname_out = fname_tmp;
  }

  FILE *fp_out;

  if (!fname_out) {
    fp_out = stdout;
  } else {
    dbg("output file %s\n", fname_out);

    fp_out = fopen(fname_out, "w");
    if (!fp_out)
      p_err("cannot create %s\n", fname_out);
  }

  if (b_pinyin)
    fprintf(fp_out, "!!pinyin\n");

  if ((fp=fopen(fname,"rb"))==NULL)
    p_err("Cannot open %s %s", fname, sys_err_strA());


  TSIN_GTAB_HEAD head;
  int phsz = 2;

  fread(&head, sizeof(head), 1, fp);
  if (!strcmp(head.signature, TSIN_EN_WORD_KEY)) {
    phsz = 1;
  } else if (!strcmp(head.signature, TSIN_GTAB_KEY)) {
    if (head.maxkey * head.keybits > 32)
      phsz = 8;
    else
      phsz = 4;
  } else
    rewind(fp);

  if (phsz==1) {
	fprintf(fp_out, TSIN_EN_WORD_KEY"\n");
  } else
  if (phsz > 2) {
    fprintf(stderr, "phsz %d keybits:%d\n", phsz, head.keybits);
    fprintf(stderr, "keymap '%s'\n", head.keymap);
    fprintf(fp_out,TSIN_GTAB_KEY" %d %d %s\n", head.keybits, head.maxkey, head.keymap+1);
  }

  u_int64_t phbuf64[MAX_PHRASE_LEN];
  char *phbuf8 = (char *)phbuf64;
  phokey_t *phbuf = (phokey_t *)phbuf64;
  u_int *phbuf32 = (u_int *)phbuf64;

  while (!feof(fp)) {
    gboolean is_deleted = FALSE;
	clen = 0;
    if (fread(&clen,1,1,fp) <= 0)
      break;

    if (!clen) {
//	  dbg("!clen\n");
      break;
	}
    gboolean en_has_str = FALSE;

    if (clen < 0) {
//	  dbg("clen < 0 %d\n", clen);
      clen = - clen;
      en_has_str = TRUE;
    }

    fread(&usecount, sizeof(usecount_t), 1, fp);
    if (!pr_usecount)
      usecount = 0;

    if (usecount < 0) {
	  dbg("is deleted clen:%d usecount:%d\n", clen, usecount);
      is_deleted = TRUE;
    }

    phbuf64[0]=0;
    fread(phbuf64, phsz, clen, fp);

    if (phsz==1) {
	  phbuf8[clen]=0;
//	  dbg("%s %d\n", phbuf8, usecount);
	}

    char tt[MAX_PHRASE_STR_LEN];

	tt[0]=0;
	if (phsz > 1 || en_has_str) {
      int ttlen=0;
      tt[0]=0;

      for(i=0;i<clen;i++) {
        char ch[CH_SZ];

        int n = fread(ch, 1, 1, fp);
        if (n<=0)
          goto stop;

        int len=utf8_sz(ch);

        fread(&ch[1], 1, len-1, fp);

        memcpy(tt+ttlen, ch, len);
        ttlen+=len;
      }

      tt[ttlen]=0;
      if (!tt[0])
        continue;
	}

    if (is_deleted) {
	  if (phsz==1) {
	    dbg("is_deleted skip %s\n", phbuf8);
	  }
      continue;
	}

	if (en_has_str) {
//	  dbg("en_has_str '%s'\n", tt);
	  continue;
	}

    gboolean minus_found = FALSE;
    if (fnames_minusN) {
      int f;
      for(f=0; f <fnames_minusN;f++) {
        int sti, edi;
        if (tsin_seek_ex(&tsin_hands[f], phbuf, clen, &sti, &edi, NULL)) {
          int k;
          for (k=sti; k < edi; k++) {
            char klen;
            usecount_t kuse;
            phokey_t ph_k[MAX_PHRASE_LEN];
            char str_k[MAX_PHRASE_STR_LEN];

            load_tsin_entry_ex(&tsin_hands[f], k, &klen, &kuse, ph_k, (unsigned char *)str_k);
            if (klen != clen)
              continue;
            if (memcmp(phbuf, ph_k, phsz * clen))
              continue;
            if (phsz > 1 && !utf8_str_eq(str_k, tt, clen))
              continue;

            minus_found = TRUE;
            goto fou;
          }
        }
      }
    }

fou:
    if (minus_found) {
//	  dbg("minus_found %s\n", tt);
      continue;
    }

    if (no_pho) {
      fprintf(fp_out, "%s\n", phbuf8);
      continue;
    }

	if (phsz==1) {
	  fprintf(fp_out, "%s", phbuf8);
	} else {
      fprintf(fp_out, "%s ", tt);
      for(i=0;i<clen;i++) {
        if (phsz==2) {
          if (b_pinyin) {
            char *t = phokey2pinyin(phbuf[i]);
//          dbg("z %s\n", t);
            fprintf(fp_out, "%s", t);
          } else
            prph2(fp_out, phbuf[i]);
        } else {
          u_int64_t k;
          if (phsz==4)
            k = phbuf32[i];
          else
            k = phbuf64[i];

          char tkey[512];
          get_keymap_str(k, head.keymap, head.keybits, tkey);
          fprintf(fp_out, "%s", tkey);
        }

        if (i!=clen-1)
          fprintf(fp_out, " ");
      }
	}

	fprintf(fp_out, phsz==1?"\t%d\n":" %d\n", usecount);
  }

stop:
  fclose(fp);
  fclose(fp_out);

  if (binary_out) {

#if UNIX
    putenv("GCIN_NO_RELOAD=");
    unix_exec(GCIN_BIN_DIR"/tsa2d32 %s %s", fname_out, fname_out_save);
#else
    _putenv("GCIN_NO_RELOAD=Y");
    win32exec_va("tsa2d32", fname_out, fname_out_save, NULL);
#endif
  }

  exit(0);
}
