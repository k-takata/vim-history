/* multibyte.c */
char_u *mb_init __ARGS((void));
int mb_get_class __ARGS((char_u *p));
int mb_class __ARGS((unsigned lead, unsigned trail));
int mb_char2len __ARGS((int c));
int mb_char2bytes __ARGS((int c, char_u *buf));
int mb_ptr2len_check __ARGS((char_u *p));
int utf_char2cells __ARGS((int c));
int mb_ptr2cells __ARGS((char_u *p));
int mb_char2cells __ARGS((int c));
int mb_off2cells __ARGS((unsigned off));
int mb_ptr2char __ARGS((char_u *p));
int utf_ptr2char __ARGS((char_u *p));
int utfc_ptr2char __ARGS((char_u *p, int *p1, int *p2));
int utfc_char2bytes __ARGS((int off, char_u *buf));
int utf_ptr2len_check __ARGS((char_u *p));
int utfc_ptr2len_check __ARGS((char_u *p));
int utf_char2len __ARGS((int c));
int utf_char2bytes __ARGS((int c, char_u *buf));
int utf_iscomposing __ARGS((int c));
void show_utf8 __ARGS((void));
int mb_head_off __ARGS((char_u *base, char_u *p));
int mb_off_next __ARGS((char_u *base, char_u *p));
int mb_tail_off __ARGS((char_u *base, char_u *p));
void mb_adjust_cursor __ARGS((void));
void mb_adjustpos __ARGS((pos_t *lp));
int mb_charlen __ARGS((char_u *str));
int mb_dec __ARGS((pos_t *lp));
int mb_isbyte1 __ARGS((char_u *buf, int x));
int mb_lefthalve __ARGS((int row, int col));
void xim_set_focus __ARGS((int focus));
void xim_set_preedit __ARGS((void));
void xim_set_status_area __ARGS((void));
void xim_init __ARGS((void));
void xim_decide_input_style __ARGS((void));
void xim_reset __ARGS((void));
int xim_queue_key_press_event __ARGS((GdkEvent *ev));
void xim_init __ARGS((void));
int xim_get_status_area_height __ARGS((void));