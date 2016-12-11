//extern int *phidx;
//extern FILE *fph;

typedef struct CHPHO {
  char *ch;
  char cha[CH_SZ+1];
  phokey_t pho;
  u_short flag;
  char psta; // phrase start index
} CHPHO;

enum {
  FLAG_CHPHO_FIXED=1,    // user selected the char, so it should not be changed
  FLAG_CHPHO_PHRASE_HEAD=2,
  FLAG_CHPHO_PHRASE_USER_HEAD=4,
  FLAG_CHPHO_PHRASE_VOID=8,
  FLAG_CHPHO_PHRASE_BODY=16,
  FLAG_CHPHO_PHO_PHRASE=32,
  FLAG_CHPHO_PINYIN_TONE=64,
  FLAG_CHPHO_GTAB_BUF_EN_NO_SPC=128,
  FLAG_CHPHO_PHRASE_TAIL=0x100,
  FLAG_CHPHO_EN_PHRASE=0x200,
};

void extract_pho(gboolean is_en, int chpho_idx, int plen, phokey_t *pho);
gboolean tsin_seek(void *pho, int plen, int *r_sti, int *r_edi, char *tone_off);
void load_tsin_entry(int idx, char *len, usecount_t *usecount, void *pho, u_char *ch);
gboolean check_fixed_mismatch(int chpho_idx, char *mtch, int plen);
gboolean tsin_pho_mode();
char *get_chpho_pinyin_set(char *set_arr);

#define TSIN_GTAB_KEY "!!!!gtab-keys"
#define TSIN_EN_WORD_KEY "!!!!tsin-en-word"

typedef struct {
  char signature[32];
  int version, flag;
  int keybits, maxkey;
  char keymap[128];
} TSIN_GTAB_HEAD;

typedef struct PRE_SEL {
  u_int64_t phkey[MAX_PHRASE_LEN];  // gtab 4-byte is actually stored as u_int not u_int64_t
//  int phidx;
  char str[MAX_PHRASE_LEN*CH_SZ+1];
  int len;
  usecount_t usecount;
} PRE_SEL;

extern gboolean tsin_is_gtab;
//extern int ph_key_sz;


typedef struct {
FILE *fph, *fp_phidx;
int phcount;
int a_phcount;
int hashidx[TSIN_HASH_N];
time_t modify_time;
char *tsin_fname;
int ph_key_sz; // bytes
gboolean tsin_is_gtab;
} TSIN_HANDLE;

extern TSIN_HANDLE tsin_hand, en_hand;

gboolean save_phrase_to_db(TSIN_HANDLE *th, void *phkeys, char *utf8str, int len, usecount_t usecount);

#define TSIN_EN_FILE "en-american"
