/* gui_w16.c */
void VimBuddyText __ARGS((char_u *msg, int mode));
int gui_is_win32s __ARGS((void));
void gui_mch_set_blinking __ARGS((long wait, long on, long off));
void gui_mch_stop_blink __ARGS((void));
void gui_mch_start_blink __ARGS((void));
int gui_mch_get_mouse_x __ARGS((void));
int gui_mch_get_mouse_y __ARGS((void));
void gui_mch_setmouse __ARGS((int x, int y));
void gui_mch_prepare __ARGS((int *argc, char **argv));
int gui_mch_init __ARGS((void));
void gui_mch_new_colors __ARGS((void));
int gui_mch_open __ARGS((void));
void gui_mch_exit __ARGS((int rc));
int gui_mch_get_winpos __ARGS((int *x, int *y));
void gui_mch_set_winpos __ARGS((int x, int y));
void gui_mch_set_shellsize __ARGS((int width, int height, int min_width, int min_height, int base_width, int base_height));
void gui_mch_get_screen_dimensions __ARGS((int *screen_w, int *screen_h));
void gui_mch_set_text_area_pos __ARGS((int x, int y, int w, int h));
void gui_mch_enable_scrollbar __ARGS((scrollbar_t *sb, int flag));
void gui_mch_set_scrollbar_thumb __ARGS((scrollbar_t *sb, long val, long size, long max));
void gui_mch_set_scrollbar_pos __ARGS((scrollbar_t *sb, int x, int y, int w, int h));
void gui_mch_create_scrollbar __ARGS((scrollbar_t *sb, int orient));
void gui_mch_destroy_scrollbar __ARGS((scrollbar_t *sb));
int gui_mch_init_font __ARGS((char_u *font_name, int fontset));
GuiFont gui_mch_get_font __ARGS((char_u *name, int giveErrorIfMissing));
void gui_mch_set_font __ARGS((GuiFont font));
void gui_mch_free_font __ARGS((GuiFont font));
guicolor_t gui_mch_get_color __ARGS((char_u *name));
void gui_mch_set_fg_color __ARGS((guicolor_t color));
void gui_mch_set_bg_color __ARGS((guicolor_t color));
void ImeSetOriginMode __ARGS((void));
void ImeSetEnglishMode __ARGS((void));
int gui_mch_adjust_charsize __ARGS((void));
void gui_mch_draw_string __ARGS((int row, int col, char_u *s, int len, int flags));
int gui_mch_haskey __ARGS((char_u *name));
void gui_mch_beep __ARGS((void));
void gui_mch_flash __ARGS((int msec));
void gui_mch_invert_rectangle __ARGS((int r, int c, int nr, int nc));
void gui_mch_iconify __ARGS((void));
void gui_mch_set_foreground __ARGS((void));
void gui_mch_settitle __ARGS((char_u *title, char_u *icon));
void gui_mch_draw_hollow_cursor __ARGS((guicolor_t color));
void gui_mch_draw_part_cursor __ARGS((int w, int h, guicolor_t color));
void gui_mch_update __ARGS((void));
int gui_mch_wait_for_chars __ARGS((int wtime));
void gui_mch_flush __ARGS((void));
void gui_mch_clear_block __ARGS((int row1, int col1, int row2, int col2));
void gui_mch_clear_all __ARGS((void));
void gui_mch_delete_lines __ARGS((int row, int num_lines));
void gui_mch_insert_lines __ARGS((int row, int num_lines));
void gui_mch_enable_menu __ARGS((int flag));
void gui_mch_set_menu_pos __ARGS((int x, int y, int w, int h));
void gui_mch_add_menu __ARGS((vimmenu_t *menu, vimmenu_t *parent, int pos));
void gui_mch_show_popupmenu __ARGS((vimmenu_t *menu));
void gui_make_tearoff __ARGS((char_u *path_name));
void gui_mch_add_menu_item __ARGS((vimmenu_t *menu, vimmenu_t *parent, int idx));
void gui_mch_destroy_menu __ARGS((vimmenu_t *menu));
void gui_mch_menu_grey __ARGS((vimmenu_t *menu, int grey));
void gui_mch_menu_hidden __ARGS((vimmenu_t *menu, int hidden));
void gui_mch_draw_menubar __ARGS((void));
int gui_mch_get_lightness __ARGS((guicolor_t pixel));
char_u *gui_mch_get_rgb __ARGS((guicolor_t pixel));
char_u *gui_mch_browse __ARGS((int saving, char_u *title, char_u *dflt, char_u *ext, char_u *initdir, char_u *filter));
int gui_mch_dialog __ARGS((int type, char_u *title, char_u *message, char_u *buttons, int dfltbutton));
void gui_mch_activate_window __ARGS((void));
void gui_mch_show_toolbar __ARGS((int showit));
void gui_simulate_alt_key __ARGS((exarg_t *eap));
void gui_mch_find_dialog __ARGS((exarg_t *eap));
void gui_mch_replace_dialog __ARGS((exarg_t *eap));
void gui_mch_mousehide __ARGS((int hide));
void mch_set_mouse_shape __ARGS((int shape));
