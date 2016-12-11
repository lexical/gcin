/*
	Copyright (C) 1995-2008	Edward Der-Hua Liu, Hsin-Chu, Taiwan
*/
typedef u_short phokey_t;

typedef struct {
  char selkeyN;
  struct {
    char num, typ;
  } phokbm[128][3];  // for 26 keys pho, it may have up-to 3 pho char for a key
} PHOKBM;

extern PHOKBM phkbm;

typedef struct {
  char ch[CH_SZ];
  int count;
} PHO_ITEM;

typedef struct {
  phokey_t key;
  u_short start;
} PHO_IDX;


typedef struct {
  char pinyin[7];
  phokey_t key;
} PIN_JUYIN;

#define MAX_PHRASE_LEN (64)
#define MAX_PHRASE_STR_LEN (MAX_PHRASE_LEN * CH_SZ + 1)

#define Min(a,b) ((a) < (b) ? (a):(b))

extern char phofname[128];
extern u_short idxnum_pho;
extern PHO_IDX *idx_pho;
extern int ch_pho_ofs;
extern PHO_ITEM *ch_pho;
extern int ch_phoN;
extern PIN_JUYIN *pin_juyin;
extern int pin_juyinN;

void pho_load();
extern char *pho_chars[];
char *phokey_to_str(phokey_t kk);
int utf8_pho_keys(char *big5, phokey_t *phkeys);
void prph(phokey_t kk);
void prphs(phokey_t *ks, int ksN);
phokey_t pho2key(char typ_pho[]);
int lookup(u_char *s);
int find_match(char *str, int *eq_N, usecount_t *usecount);
char *phokey_to_str2(phokey_t kk, int last_number);
char *pho_idx_str(int idx);
char *pho_idx_str2(int idx, int *is_phrase);


#define MAX_PH_BF (90)

#define MAX_PH_BF_EXT (MAX_PH_BF + MAX_PHRASE_LEN + 1)

#define TSIN_HASH_N (256)
#define TSIN_HASH_SHIFT8 0  // doesn't need shift
#define TSIN_HASH_SHIFT 6
#define TSIN_HASH_SHIFT_32 24
#define TSIN_HASH_SHIFT_64 56

#define PHO_CHAR_LEN 3

#define L_BRACKET_NO 22
#define R_BRACKET_NO 23
#define BACK_QUOTE_NO 24

#define PHO_PHRASE_ESCAPE 0x1b
#define PHO_PINYIN_TONE1 -1
