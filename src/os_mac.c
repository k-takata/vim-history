/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * os_mac.c -- code for the MacOS
 *
 * This file manly based on os_unix.c.
 */

#include "vim.h"
#include "globals.h"
#include "option.h"
#include "proto.h"
#include <unistd.h>
#include <utsname.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#include <Memory.h>
#include <OSUtils.h>
#include <Files.h>


/*
 * Recursively build up a list of files in "gap" matching the first wildcard
 * in `path'.  Called by expand_wildcards().
 */
    int
mch_expandpath(
    struct growarray	*gap,
    char_u		*path,
    int			flags)
{
    /*
     * TODO: +Support for unix path type
     *	      (within caller??)
     *       +Get Volumes (when looking for files in current dir)
     *       +Make it work when working dir not on select volume
     *       +Cleanup
     */
    short       index = 1;
    OSErr       gErr;
    char_u      dirname[256];
    char_u      cfilename[256];
    long	dirID;
    char_u      *new_name;
    CInfoPBRec  gMyCPB;

    char_u	    *buf;
    char_u	    *p, *s, *e;
    int		    start_len, c;
    char	    dummy;
    char_u	    *pat;
    vim_regexp	*prog;
    int		    matches;

    start_len = gap->ga_len;
    buf = alloc(STRLEN(path) + BASENAMELEN + 5);/* make room for file name */
    if (buf == NULL)
    	return 0;

/*
 * Find the first part in the path name that contains a wildcard.
 * Copy it into buf, including the preceding characters.
 */
    p = buf;
    s = NULL;
    e = NULL;
    while (*path)
    {
	    if (*path == ':')
	    {
    	    if (e)
			break;
		else
			s = p;
	    }
	    if (vim_strchr((char_u *)"*?[{~$", *path) != NULL)
		e = p;
    	*p++ = *path++;
    }
    e = p;
    if (s != NULL)
	    s++;
    else
	    s = buf;

    /* now we have one wildcard component between s and e */
    *e = NUL;

    /* convert the file pattern to a regexp pattern */
    pat = file_pat_to_reg_pat(s, e, NULL);
    if (pat == NULL)
    {
	    vim_free(buf);
	    return 0;
    }

    /* compile the regexp into a program */
    reg_ic = FALSE;				/* Don't ever ignore case */
    prog = vim_regcomp(pat, TRUE);
    vim_free(pat);

    if (prog == NULL)
    {
	    vim_free(buf);
	    return 0;
    }

    /* open the directory for scanning */
    c = *s;
    *s = NUL;

    if (*buf == NUL)
    {
	(void) mch_dirname (&dirname[1], 254);
	dirname[0] = STRLEN(&dirname[1]);
    }
    else
    {
	if (*buf == ':')  /* relative path */
	{
	    (void) mch_dirname (&dirname[1], 254);
	    new_name=concat_fnames(&dirname[1], buf+1, TRUE);
	    STRCPY(&dirname[1], new_name);
	    dirname[0] = STRLEN(new_name);
	    vim_free(new_name);
	}
	else
	{
		STRCPY(&dirname[1], buf);
	    backslash_halve (&dirname[1], TRUE);
	    dirname[0] = STRLEN(buf);
	}
    }
    *s = c;

    gMyCPB.dirInfo.ioNamePtr    = dirname;
    gMyCPB.dirInfo.ioVRefNum    = 0;
    gMyCPB.dirInfo.ioFDirIndex  = 0;
    gMyCPB.dirInfo.ioDrDirID    = 0;

    gErr = PBGetCatInfo(&gMyCPB, false);

    gMyCPB.dirInfo.ioCompletion = NULL;
    dirID = gMyCPB.dirInfo.ioDrDirID;

    do
    {
	gMyCPB.hFileInfo.ioFDirIndex = index;
	gMyCPB.hFileInfo.ioDirID     = dirID;

	gErr = PBGetCatInfo(&gMyCPB,false);

	if (gErr == noErr)
	{
	    STRNCPY (cfilename, &dirname[1], dirname[0]);
	    cfilename[dirname[0]] = 0;
	    if (vim_regexec(prog, cfilename, TRUE))
	    {
		if (s[-1] != ':')
		{
		    STRCPY(s+1, cfilename);
		    s[0] = ':';
		}
		else
		{
		    STRCPY(s, cfilename);
		}
		STRCAT(buf, path);
		if (mch_has_wildcard(path))	/* handle more wildcard */
		    (void)mch_expandpath(gap, buf, flags);
		else
		    addfile(gap, buf, flags);
	    }
	    if ((gMyCPB.hFileInfo.ioFlAttrib & ioDirMask) !=0 )
	    {
	    }
	    else
	    {
	    }
	}
	index++;
    }
    while (gErr == noErr);

    return gap->ga_len - start_len;
}

    int
vim_chdir (x)
    char    *x;
{
  return (chdir (x));
}

    int
vim_remove (x)
    unsigned char *x;
{
  return (unlink((char *)x));
}

    void
fname_case (name)
    char_u  *name;
{
    /*
     * TODO: get the real casing for the file
     *       make it called
     *       with USE_FNAME_CASE & USE_LONG_FNAME
     *		    CASE_INSENSITIVE_FILENAME
     *       within setfname, fix_fname, do_ecmd
     */
}
/*
static int	WaitForChar __ARGS((long));
static int	RealWaitForChar __ARGS((int, long));
static int		do_resize = FALSE;
*/
static char_u	*oldtitle = NULL;
/*
static char_u	*fixedtitle = (char_u *)"Thanks for flying Vim";
static char_u	*oldicon = NULL;
*/

/*
 * check for an "interrupt signal": CTRL-break or CTRL-C
 */
    void
mch_breakcheck()
{
    /*
     * TODO: Scan event for a CTRL-C or COMMAND-. and do: got_int=TRUE;
     *       or only go proccess event?
     *       or do nothing
     */
}

    long_u
mch_avail_mem(special)
    int     special;
{
    /*
     * TODO: Use MaxBlock, FreeMeM, PurgeSpace, MaxBlockSys FAQ-266
     *       figure out what the special is for
     */
    return 0x7fffffff; /* in bytes */
}

    void
mch_delay(msec, ignoreinput)
    long	msec;
    int		ignoreinput;
{
    long   finalTick;

    if (ignoreinput)
	Delay (60*msec/1000, &finalTick);
    else
	/* even thougth we should call gui stuff from here
	   it the simplest way to be safe */
	gui_mch_wait_for_chars(msec);
}

    void
mch_windinit()
{
    /*
     *  TODO: Verify if needed, or override later.
     */
    Columns = 80;
    Rows = 24;
}

/*
 * Check_win checks whether we have an interactive stdout.
 */
    int
mch_check_win(argc, argv)
    int		argc;
    char	**argv;
{
    /*
     * TODO: Maybe to be remove through NO_CONSOLE
     */
    return OK;
}

/*
 * Return TRUE if the input comes from a terminal, FALSE otherwise.
 */
    int
mch_input_isatty()
{
    /*
     * TODO: Maybe to be remove through NO_CONSOLE
     */
    return OK;
}

/*
 * Set the window title and icon.
 * (The icon is not taken care of).
 */
    void
mch_settitle(title, icon)
    char_u *title;
    char_u *icon;
{
    /*
     *  TODO: Clean C-Pscal conversion
     */
    char_u   pascal_title[1024];

    if (title == NULL)		/* nothing to do */
	return;

    if (title != NULL)
    {
	pascal_title[0] = (char_u) STRLEN(title);
	pascal_title[1] = 0;
	STRCAT (&pascal_title, title);
	SetWTitle(gui.VimWindow, (char_u *) &pascal_title);
    }
}

/*
 * Restore the window/icon title.
 * which is one of:
 *	1  Just restore title
 *      2  Just restore icon
 *	3  Restore title and icon
 * but don't care about the icon.
 */
    void
mch_restore_title(which)
    int which;
{
    mch_settitle((which & 1) ? oldtitle : NULL, NULL);
}

/*
 * Insert user name in s[len].
 * Return OK if a name found.
 */
	int
mch_get_user_name(s, len)
	char_u	*s;
	int		len;
{
	/*
	 * TODO: clean up and try getlogin ()
	 */
#if defined(HAVE_PWD_H) && defined(HAVE_GETPWUID)
	struct passwd	*pw;
#endif
	uid_t			uid;

	uid = getuid();
#if defined(HAVE_PWD_H) && defined(HAVE_GETPWUID)
	if ((pw = getpwuid(uid)) != NULL &&
								 pw->pw_name != NULL && *(pw->pw_name) != NUL)
	{
		STRNCPY(s, pw->pw_name, len);
		return OK;
	}
#endif
	sprintf((char *)s, "%d", (int)uid);		/* assumes s is long enough */
	return FAIL;							/* a number is not a name */
}

/*
 * Insert host name is s[len].
 */

    void
mch_get_host_name(s, len)
    char_u	*s;
    int		len;
{
    struct utsname vutsname;

    uname(&vutsname);
    STRNCPY(s, vutsname.nodename, len);
}

/*
 * return process ID
 */
    long
mch_get_pid()
{
    return (long)getpid();
}

/*
 * Get name of current directory into buffer 'buf' of length 'len' bytes.
 * Return OK for success, FAIL for failure.
 */
    int
mch_dirname(buf, len)
    char_u	*buf;
    int		len;
{
    /* The last : is already put by getcwd */
    if (getcwd((char *)buf, len) == NULL)
	{
	    STRCPY(buf, strerror(errno));
	    return FAIL;
	}
    return OK;
}

    void
slash_n_colon_adjust (buf)
    char_u *buf;
{
    /*
     * TODO: Make it faster
     */

    char_u  temp[MAXPATHL];
    char_u  *dot;
    char_u  *slash;
    char_u  *searching;
    char_u  *first_colon = vim_strchr(buf, ':');
    char_u  *first_slash = vim_strchr(buf, '/');
    int     full = TRUE;
    char_u  *scanning;
    char_u  *filling;
    char_u  last_copied = NUL;

    if (*buf == NUL)
	return ;

    if ((first_colon == NULL) && (first_slash == NULL))
	full = FALSE;
    if ((first_slash == NULL) && (first_colon != NULL))
	full = TRUE;
    if ((first_colon == NULL) && (first_slash != NULL))
	full =	FALSE;
    if ((first_slash < first_colon) && (first_slash != NULL))
	full = FALSE;
    if ((first_colon < first_slash) && (first_colon != NULL))
	full = TRUE;
    if (first_slash == buf)
	full = TRUE;
    if (first_colon == buf)
	full = FALSE;

    scanning = buf;
    filling  = temp;

    while (*scanning != NUL)
    {
	if (*scanning == '/')
	{
	    if (scanning[1] != '/')
	    {
		*filling++ = ':';
		scanning++;
	    }
	    else
		scanning++;
	}
	else if (*scanning == '.')
	{
	    if ((scanning[1] == NUL) || scanning[1] == '/')
	    {
		if (scanning[1] == NUL)
		    scanning += 1;
		else
		    scanning += 2;
	    }
	    else if (scanning[1] == '.')
	    {
		if ((scanning[2] == NUL) || scanning[2] == '/')
		{
		    *filling++ = ':';
		    if (scanning[2] == NUL)
			scanning +=2;
		    else
			scanning += 3;
		}
		else
		{
		    *filling++ = *scanning++;
		}
	    }
	    else
	    {
		*filling++ = *scanning++;
	    }

	}
	else
	{
	    *filling++ = *scanning++;
	}

    }

    *filling = 0;
    filling = temp;

    if (!full)
    {
	if (buf[0] != ':')
	{
	    buf[0] = ':';
	    buf[1] = NUL;
	}
	else
	    buf[0] = NUL;
    }
    else
    {
	buf[0] = NUL;
	if (filling[0] == ':')
	    filling++;
    }

    STRCAT (buf, filling);

}

/*
 * Get absolute filename into buffer 'buf' of length 'len' bytes.
 *
 * return FAIL for failure, OK for success
 */
    int
mch_FullName(fname, buf, len, force)
    char_u  *fname, *buf;
    int     len;
    int     force;	    /* also expand when already absolute path name */
{

    /*
     * TODO: Find what TODO
     */
	int		l;
	char_u	olddir[MAXPATHL];
	char_u   newdir[MAXPATHL];
	char_u	*p;
	char_u	c;
	int		retval = OK;

	if (fname == NULL)	/* always fail */
	{
		*buf = NUL;
		return FAIL;
	}

	*buf = 0;
	if (force || !mch_isFullName(fname))	/* if forced or not an absolute path */
	{
		/*
		 * If the file name has a path, change to that directory for a moment,
		 * and then do the getwd() (and get back to where we were).
		 * This will get the correct path name with "../" things.
		 */

/*		STRCAT(buf, fname);
		slash_n_colon_adjust (buf);
		fname[0] = 0;
		STRCAT(fname, buf);
*/
		if ((p = vim_strrchr(fname, ':')) != NULL)
		{
			if (mch_dirname(olddir, MAXPATHL) == FAIL)
			{
				p = NULL;		/* can't get current dir: don't chdir */
				retval = FAIL;
			}
			else
			{
				c = *p;
				*p = NUL;
				if (vim_chdir((char *)fname))
					retval = FAIL;
				else
					fname = p + 1;
				*p = c;
			}
		}
		if (mch_dirname(buf, len) == FAIL)
		{
			retval = FAIL;
			*newdir = NUL;
		}
		l = STRLEN(buf);
		if (l && buf[l - 1] != ':') /*MAC*/
			STRCAT(buf, ":");
		if (p != NULL)
		{
				vim_chdir((char *)olddir);
		}
	}
	else
	{
		STRCAT(buf, fname);
		slash_n_colon_adjust (buf);
		fname[0] = 0;
		STRCAT(fname, buf);
		buf[0] = 0;
	}
	STRCAT(buf, fname);
	slash_adjust(buf);
	return retval;
}

/*
 * return TRUE is fname is an absolute path name
 */
	int
mch_isFullName(fname)
	char_u		*fname;
{
    /*
     * TODO: Make sure fname is always of mac still
     *       i.e: passed throught slash_n_colon_adjust
     */
	char_u	*first_colon = vim_strchr(fname, ':');
	char_u	*first_slash = vim_strchr(fname, '/');

	if (first_colon == fname)
	  return FALSE;
	if (first_slash == fname)
	  return TRUE;
	if ((first_colon < first_slash) && (first_colon != NULL))
	  return TRUE;
	if ((first_slash < first_colon) && (first_slash != NULL))
	  return FALSE;
	if ((first_colon == NULL) && (first_slash != NULL))
	  return FALSE;
	if ((first_slash == NULL) && (first_colon != NULL))
	  return TRUE;
	if ((first_colon == NULL) && (first_slash == NULL))
	  return FALSE;
	return TRUE;
}

/*
 * Replace all slashes by colons.
 */
    void
slash_adjust(p)
    char_u  *p;
{
    /*
     * TODO: keep escaped '/'
     */

    while (*p)
    {
	if (*p == '/')
	    *p = ':';
	++p;
    }
}

/*
 * Get file permissions for 'name'.
 * Returns -1 when it doesn't exist.
 */
    long
mch_getperm(name)
    char_u *name;
{
    /*
     * TODO: Maybe use AppleShare info??
     *       Use locked for non writable
     */

    struct stat statb;

    if (stat((char *)name, &statb))
	return -1;
    return statb.st_mode;
}

/*
 * set file permission for 'name' to 'perm'
 *
 * return FAIL for failure, OK otherwise
 */
    int
mch_setperm(name, perm)
    char_u  *name;
    int     perm;
{
    /*
     * TODO: Maybe use AppleShare info??
     *       Use locked for non writable
     */
    return (OK);
}

/*
 * Set hidden flag for "name".
 */
    void
mch_hide(name)
    char_u  *name;
{
    /*
     * TODO: Hide the file throught FileManager FAQ 8-34
     *
     *  *name is mac style start with : for relative
     */
}


/*
 * return TRUE if "name" is a directory
 * return FALSE if "name" is not a directory
 * return FALSE for error
 */
    int
mch_isdir(name)
    char_u  *name;
{
    /*
     * TODO: Find maybe throught FileManager calls
     */

    struct stat statb;

    if (stat((char *)name, &statb))
	return FALSE;
    return ((statb.st_mode & S_IFMT) == S_IFDIR ? TRUE : FALSE);
}

    void
mch_windexit(r)
    int     r;
{
    mch_display_error();

    ml_close_all(TRUE);		/* remove all memfiles */
    exit(r);
}

static int curr_tmode = TMODE_COOK;     /* contains current terminal mode */

    void
mch_settmode(tmode)
    int     tmode;
{
    /*
     * TODO: remove the needs of it.
     */
}

#ifdef USE_MOUSE
/*
 * set mouse clicks on or off (only works for xterms)
 */
    void
mch_setmouse(on)
    int     on;
{
    /*
     * TODO: remove the needs of it.
     */
}
#endif

/*
 * set screen mode, always fails.
 */
    int
mch_screenmode(arg)
    char_u	 *arg;
{
    EMSG("Screen mode setting not supported");
    return FAIL;
}

    int
mch_call_shell(cmd, options)
    char_u  *cmd;
    int	    options;	/* SHELL_FILTER if called by do_filter() */
			/* SHELL_COOKED if term needs cooked mode */
{
    /*
     * TODO: find a shell or pseudo-shell to call
     *       for some simple useful command
     */

    return (FAIL);
}

	int
mch_has_wildcard(p)
	char_u	*p;
{
    for ( ; *p; ++p)
    {
	if (*p == '\\' && p[1] != NUL)
	    ++p;
	else if (vim_strchr((char_u *) WILDCARD_LIST, *p) != NULL)
	    return TRUE;
    }
    return FALSE;
}

