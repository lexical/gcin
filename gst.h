typedef struct {
  struct CHPHO *chpho;
  int c_idx, c_len;
  int ph_sta;  // phrase start
  int sel_pho;
  int current_page;
  int startf, pho_count, phrase_count;
  gboolean full_match;
  gboolean tsin_half_full;
  gboolean tsin_buffer_editing;
  gboolean ctrl_pre_sel;
  struct PRE_SEL *pre_sel;
  int pre_selN;
  int last_cursor_idx;
  int pho_menu_idx;
} TSIN_ST;
extern TSIN_ST tss;

typedef enum {
  SAME_PHO_QUERY_none = 0,
  SAME_PHO_QUERY_gtab_input = 1,
  SAME_PHO_QUERY_pho_select = 2,
} SAME_PHO_QUERY;

typedef struct {
  int ityp3_pho;
  int cpg, maxi;
  int start_idx, stop_idx;
  char typ_pho[4];
  char inph[8];
  SAME_PHO_QUERY same_pho_query_state;
} PHO_ST;
extern PHO_ST poo;

#define MAX_TAB_KEY_NUM64_6 (10)

typedef struct {
  int S1, E1, last_idx, wild_page, pg_idx, total_matchN, sel1st_i;
  u_int64_t kval;
  gboolean last_full, wild_mode, spc_pressed, invalid_spc, more_pg, gtab_buf_select;
  short defselN, exa_match, ci, gbufN, gbuf_cursor;
  KeySym inch[MAX_TAB_KEY_NUM64_6];
} GTAB_ST;
extern GTAB_ST ggg;
