/* gui_w32.c */
void gui_mch_set_blinking __ARGS((long wait, long on, long off));
void gui_mch_stop_blink __ARGS((void));
void gui_mch_start_blink __ARGS((void));
void gui_mch_prepare __ARGS((int *argc, char **argv));
int gui_mch_init __ARGS((void));
void gui_mch_new_colors __ARGS((void));
int gui_mch_open __ARGS((void));
void gui_mch_exit __ARGS((int rc));
void gui_mch_set_winsize __ARGS((int width, int height, int min_width, int min_height, int base_width, int base_height));
void gui_mch_get_screen_dimensions __ARGS((int *screen_w, int *screen_h));
void gui_mch_set_text_area_pos __ARGS((int x, int y, int w, int h));
void gui_mch_enable_scrollbar __ARGS((GuiScrollbar *sb, int flag));
void gui_mch_set_scrollbar_thumb __ARGS((GuiScrollbar *sb, int val, int size, int max));
void gui_mch_set_scrollbar_pos __ARGS((GuiScrollbar *sb, int x, int y, int w, int h));
void gui_mch_create_scrollbar __ARGS((GuiScrollbar *sb, int orient));
void gui_mch_destroy_scrollbar __ARGS((GuiScrollbar *sb));
int gui_mch_init_font __ARGS((char_u *font_name));
GuiFont gui_mch_get_font __ARGS((char_u *name, int giveErrorIfMissing));
void gui_mch_set_font __ARGS((GuiFont font));
int gui_mch_same_font __ARGS((GuiFont f1, GuiFont f2));
void gui_mch_free_font __ARGS((GuiFont font));
GuiColor gui_mch_get_color __ARGS((char_u *name));
void gui_mch_set_fg_color __ARGS((GuiColor color));
void gui_mch_set_bg_color __ARGS((GuiColor color));
void gui_mch_draw_string __ARGS((int row, int col, char_u *s, int len, int flags));
int gui_mch_haskey __ARGS((char_u *name));
void gui_mch_beep __ARGS((void));
void gui_mch_flash __ARGS((void));
void gui_mch_invert_rectangle __ARGS((int r, int c, int nr, int nc));
void gui_mch_iconify __ARGS((void));
void gui_mch_set_foreground __ARGS((void));
void gui_mch_settitle __ARGS((char_u *title, char_u *icon));
void gui_mch_draw_hollow_cursor __ARGS((GuiColor color));
void gui_mch_draw_part_cursor __ARGS((int w, int h, GuiColor color));
void gui_mch_update __ARGS((void));
int gui_mch_wait_for_chars __ARGS((int wtime));
void gui_mch_flush __ARGS((void));
void gui_mch_clear_block __ARGS((int row1, int col1, int row2, int col2));
void gui_mch_clear_all __ARGS((void));
void gui_mch_delete_lines __ARGS((int row, int num_lines));
void gui_mch_insert_lines __ARGS((int row, int num_lines));
void gui_mch_enable_menu __ARGS((int flag));
void gui_mch_set_menu_pos __ARGS((int x, int y, int w, int h));
void gui_mch_add_menu __ARGS((GuiMenu *menu, GuiMenu *parent));
void gui_mch_add_menu_item __ARGS((GuiMenu *menu, GuiMenu *parent));
void gui_mch_destroy_menu __ARGS((GuiMenu *menu));
void gui_mch_menu_grey __ARGS((GuiMenu *menu, int grey));
void gui_mch_menu_hidden __ARGS((GuiMenu *menu, int hidden));
void gui_mch_draw_menubar __ARGS((void));
int gui_mch_get_lightness __ARGS((GuiColor pixel));
char_u *gui_mch_get_rgb __ARGS((GuiColor pixel));
