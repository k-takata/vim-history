/* charset.c */
int init_chartab __ARGS((void));
void trans_characters __ARGS((char_u *buf, int bufsize));
char_u *transchar __ARGS((int c));
void transchar_nonprint __ARGS((char_u *buf, int c));
int charsize __ARGS((int c));
int vim_strsize __ARGS((char_u *s));
int chartabsize __ARGS((int c, colnr_t col));
int win_chartabsize __ARGS((WIN *wp, int c, colnr_t col));
int linetabsize __ARGS((char_u *s));
int win_linetabsize __ARGS((WIN *wp, char_u *s));
int vim_isIDc __ARGS((int c));
int vim_iswordc __ARGS((int c));
int vim_iswordc_buf __ARGS((int c, BUF *buf));
int vim_isfilec __ARGS((int c));
int vim_isprintc __ARGS((int c));
int safe_vim_isprintc __ARGS((int c));
int lbr_chartabsize __ARGS((unsigned char *s, colnr_t col));
int win_lbr_chartabsize __ARGS((WIN *wp, unsigned char *s, colnr_t col, int *head));
void getvcol __ARGS((WIN *wp, FPOS *pos, colnr_t *start, colnr_t *cursor, colnr_t *end));
void getvcols __ARGS((FPOS *pos1, FPOS *pos2, colnr_t *left, colnr_t *right));
char_u *skipwhite __ARGS((char_u *p));
char_u *skipdigits __ARGS((char_u *p));
int vim_isdigit __ARGS((int c));
char_u *skiptowhite __ARGS((char_u *p));
char_u *skiptowhite_esc __ARGS((char_u *p));
long getdigits __ARGS((char_u **pp));
int vim_isblankline __ARGS((char_u *lbuf));
void vim_str2nr __ARGS((char_u *start, int *hexp, int *len, int dooct, int dohex, long *nptr, unsigned long *unptr));
int hex2nr __ARGS((int c));
