/* os_win32.c */
int dyn_libintl_init __ARGS((char *libname));
void dyn_libintl_end __ARGS((void));
void PlatformId __ARGS((void));
int mch_windows95 __ARGS((void));
void mch_setmouse __ARGS((int on));
void mch_update_cursor __ARGS((void));
int mch_char_avail __ARGS((void));
int mch_inchar __ARGS((char_u *buf, int maxlen, long time, int tb_change_cnt));
void mch_init __ARGS((void));
void mch_exit __ARGS((int r));
int mch_check_win __ARGS((int argc, char **argv));
void fname_case __ARGS((char_u *name, int len));
int mch_get_user_name __ARGS((char_u *s, int len));
void mch_get_host_name __ARGS((char_u *s, int len));
long mch_get_pid __ARGS((void));
int mch_dirname __ARGS((char_u *buf, int len));
long mch_getperm __ARGS((char_u *name));
int mch_setperm __ARGS((char_u *name, long perm));
void mch_hide __ARGS((char_u *name));
int mch_isdir __ARGS((char_u *name));
int mch_writable __ARGS((char_u *name));
int mch_can_exe __ARGS((char_u *name));
int mch_nodetype __ARGS((char_u *name));
vim_acl_T mch_get_acl __ARGS((char_u *fname));
void mch_set_acl __ARGS((char_u *fname, vim_acl_T acl));
void mch_free_acl __ARGS((vim_acl_T acl));
void mch_settmode __ARGS((int tmode));
int mch_get_shellsize __ARGS((void));
void mch_set_shellsize __ARGS((void));
void mch_new_shellsize __ARGS((void));
void mch_set_winsize_now __ARGS((void));
int mch_call_shell __ARGS((char_u *cmd, int options));
void mch_set_normal_colors __ARGS((void));
void mch_write __ARGS((char_u *s, int len));
void mch_delay __ARGS((long msec, int ignoreinput));
int mch_remove __ARGS((char_u *name));
void mch_breakcheck __ARGS((void));
long_u mch_avail_mem __ARGS((int special));
int mch_rename __ARGS((const char *pszOldFile, const char *pszNewFile));
char *default_shell __ARGS((void));
int mch_access __ARGS((char *n, int p));
int mch_copy_file_attribute __ARGS((char_u *from, char_u *to));
int myresetstkoflw __ARGS((void));
/* vim: set ft=c : */
