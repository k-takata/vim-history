/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * misc1.c: functions that didn't seem to fit elsewhere
 */

#include "vim.h"
#include "version.h"

#ifdef HAVE_FCNTL_H
# include <fcntl.h>		/* for chdir() */
#endif

static char_u *vim_version_dir __ARGS((char_u *vimdir));
static char_u *remove_tail __ARGS((char_u *p, char_u *pend, char_u *name));
#if defined(USE_EXE_NAME) && defined(MACOS_X)
static char_u *remove_tail_with_ext __ARGS((char_u *p, char_u *pend, char_u *name));
#endif
static int get_indent_str __ARGS((char_u *ptr, int ts));
static int temporary_nolist __ARGS((void));

/*
 * Count the size (in window cells) of the indent in the current line.
 */
    int
get_indent()
{
    return get_indent_str(ml_get_curline(), (int)curbuf->b_p_ts);
}

/*
 * Count the size (in window cells) of the indent in line "lnum".
 */
    int
get_indent_lnum(lnum)
    linenr_T	lnum;
{
    return get_indent_str(ml_get(lnum), (int)curbuf->b_p_ts);
}

#if defined(FEAT_FOLDING) || defined(PROTO)
/*
 * Count the size (in window cells) of the indent in line "lnum" of buffer
 * "buf".
 */
    int
get_indent_buf(buf, lnum)
    buf_T	*buf;
    linenr_T	lnum;
{
    return get_indent_str(ml_get_buf(buf, lnum, FALSE), (int)buf->b_p_ts);
}
#endif

/*
 * count the size (in window cells) of the indent in line "ptr", with
 * 'tabstop' at "ts"
 */
    static int
get_indent_str(ptr, ts)
    char_u	*ptr;
    int		ts;
{
    int		count = 0;

    for ( ; *ptr; ++ptr)
    {
	if (*ptr == TAB)    /* count a tab for what it is worth */
	    count += ts - (count % ts);
	else if (*ptr == ' ')
	    ++count;		/* count a space for one */
	else
	    break;
    }
    return (count);
}

/*
 * Set the indent of the current line.
 * Leaves the cursor on the first non-blank in the line.
 * Caller must take care of undo.
 * "flags":
 *	SIN_CHANGED:	call changed_bytes() if the line was changed.
 *	SIN_INSERT:	insert the indent in front of the line.
 *	SIN_UNDO:	save line for undo before changing it.
 * Returns TRUE if the line was changed.
 */
    int
set_indent(size, flags)
    int		size;
    int		flags;
{
    char_u	*p;
    char_u	*line;
    char_u	*s;
    int		todo;
    int		ind_len;
    int		line_len;
    int		doit = FALSE;

    /*
     * First check if there is anything to do and compute the number of
     * characters needed for the indent.
     */
    todo = size;
    ind_len = 0;
    p = ml_get_curline();
    if (!curbuf->b_p_et)	/* if 'expandtab' isn't set: use TABs */
	while (todo >= (int)curbuf->b_p_ts)
	{
	    if (*p != TAB)
		doit = TRUE;
	    else
		++p;
	    todo -= curbuf->b_p_ts;
	    ++ind_len;
	}
    while (todo > 0)
    {
	if (*p != ' ')
	    doit = TRUE;
	else
	    ++p;
	--todo;
	++ind_len;
    }

    /* Return if the indent is OK already. */
    if (!doit && !vim_iswhite(*p) && !(flags & SIN_INSERT))
	return FALSE;

    /* Allocate memory for the new line. */
    if (flags & SIN_INSERT)
	p = ml_get_curline();
    else
	p = skipwhite(p);
    line_len = (int)STRLEN(p) + 1;
    line = alloc(ind_len + line_len);
    if (line == NULL)
	return FALSE;

    /* Put the characters in the new line. */
    s = line;
    todo = size;
    if (!curbuf->b_p_et)	/* if 'expandtab' isn't set: use TABs */
	while (todo >= (int)curbuf->b_p_ts)
	{
	    *s++ = TAB;
	    todo -= (int)curbuf->b_p_ts;
	}
    while (todo > 0)
    {
	*s++ = ' ';
	--todo;
    }
    mch_memmove(s, p, (size_t)line_len);

    /* Replace the line (unless undo fails). */
    if (!(flags & SIN_UNDO) || u_savesub(curwin->w_cursor.lnum) == OK)
    {
	ml_replace(curwin->w_cursor.lnum, line, FALSE);
	if (flags & SIN_CHANGED)
	    changed_bytes(curwin->w_cursor.lnum, 0);
    }
    else
	vim_free(line);

    curwin->w_cursor.col = ind_len;
    return TRUE;
}

/*
 * Return the indent of the current line after a number.  Return -1 if no
 * number was found.  Used for '1' in 'formatoptions': numbered list.
 */
    int
get_number_indent(lnum)
    linenr_T	lnum;
{
    char_u	*line;
    char_u	*p;
    colnr_T	col;
    pos_T	pos;

    if (lnum > curbuf->b_ml.ml_line_count)
	return -1;
    line = ml_get(lnum);
    p = skipwhite(line);
    if (!isdigit(*p))
	return -1;
    p = skipdigits(p);
    if (vim_strchr((char_u *)":.)]}\t ", *p) == NULL)
	return -1;
    p = skipwhite(p + 1);
    if (*p == NUL)
	return -1;
    pos.lnum = lnum;
    pos.col = (colnr_T)(p - line);
    getvcol(curwin, &pos, &col, NULL, NULL);
    return (int)col;
}

#if defined(FEAT_CINDENT) || defined(FEAT_SMARTINDENT)

static int cin_is_cinword __ARGS((char_u *line));

/*
 * Return TRUE if the string "line" starts with a word from 'cinwords'.
 */
    static int
cin_is_cinword(line)
    char_u	*line;
{
    char_u	*cinw;
    char_u	*cinw_buf;
    int		cinw_len;
    int		retval = FALSE;
    int		len;

    cinw_len = (int)STRLEN(curbuf->b_p_cinw) + 1;
    cinw_buf = alloc((unsigned)cinw_len);
    if (cinw_buf != NULL)
    {
	line = skipwhite(line);
	for (cinw = curbuf->b_p_cinw; *cinw; )
	{
	    len = copy_option_part(&cinw, cinw_buf, cinw_len, ",");
	    if (STRNCMP(line, cinw_buf, len) == 0
		    && (!vim_iswordc(line[len]) || !vim_iswordc(line[len - 1])))
	    {
		retval = TRUE;
		break;
	    }
	}
	vim_free(cinw_buf);
    }
    return retval;
}
#endif

/*
 * open_line: Add a new line below or above the current line.
 *
 * For VREPLACE mode, we only add a new line when we get to the end of the
 * file, otherwise we just start replacing the next line.
 *
 * Caller must take care of undo.  Since VREPLACE may affect any number of
 * lines however, it may call u_save_cursor() again when starting to change a
 * new line.
 *
 * Return TRUE for success, FALSE for failure
 */
    int
open_line(dir, del_spaces, old_indent)
    int		dir;		/* FORWARD or BACKWARD */
    int		del_spaces;	/* delete spaces after cursor */
    int		old_indent;	/* indent for after ^^D in Insert mode */
{
    char_u	*saved_line;		/* copy of the original line */
    char_u	*next_line = NULL;	/* copy of the next line */
    char_u	*p_extra = NULL;	/* what goes to next line */
    pos_T	old_cursor;		/* old cursor position */
    int		newcol = 0;		/* new cursor column */
    int		newindent = 0;		/* auto-indent of the new line */
    int		n;
    int		trunc_line = FALSE;	/* truncate current line afterwards */
    int		retval = FALSE;		/* return value, default is FAIL */
#ifdef FEAT_COMMENTS
    int		extra_len = 0;		/* length of p_extra string */
    int		lead_len;		/* length of comment leader */
    char_u	*lead_flags;	/* position in 'comments' for comment leader */
    char_u	*leader = NULL;		/* copy of comment leader */
#endif
    char_u	*allocated = NULL;	/* allocated memory */
    char_u	*p;
    int		saved_char = NUL;	/* init for GCC */
#if defined(FEAT_SMARTINDENT) || defined(FEAT_COMMENTS)
    pos_T	*pos;
#endif
#ifdef FEAT_SMARTINDENT
    int		do_si = (!p_paste && curbuf->b_p_si
# ifdef FEAT_CINDENT
					&& !curbuf->b_p_cin
# endif
			);
    int		no_si = FALSE;		/* reset did_si afterwards */
    int		first_char = NUL;	/* init for GCC */
#endif
#if defined(FEAT_LISP) || defined(FEAT_CINDENT)
    int		vreplace_mode;
#endif
    int		did_append;		/* appended a new line */

    /*
     * make a copy of the current line so we can mess with it
     */
    saved_line = vim_strsave(ml_get_curline());
    if (saved_line == NULL)	    /* out of memory! */
	return FALSE;

    if (State & VREPLACE_FLAG)
    {
	/*
	 * With VREPLACE we make a copy of the next line, which we will be
	 * starting to replace.  First make the new line empty and let vim play
	 * with the indenting and comment leader to its heart's content.  Then
	 * we grab what it ended up putting on the new line, put back the
	 * original line, and call ins_char() to put each new character onto
	 * the line, replacing what was there before and pushing the right
	 * stuff onto the replace stack.  -- webb.
	 */
	if (curwin->w_cursor.lnum < orig_line_count)
	    next_line = vim_strsave(ml_get(curwin->w_cursor.lnum + 1));
	else
	    next_line = vim_strsave((char_u *)"");
	if (next_line == NULL)	    /* out of memory! */
	    goto theend;

	/*
	 * In VREPLACE mode, a NL replaces the rest of the line, and starts
	 * replacing the next line, so push all of the characters left on the
	 * line onto the replace stack.  We'll push any other characters that
	 * might be replaced at the start of the next line (due to autoindent
	 * etc) a bit later.
	 */
	replace_push(NUL);  /* Call twice because BS over NL expects it */
	replace_push(NUL);
	p = saved_line + curwin->w_cursor.col;
	while (*p != NUL)
	    replace_push(*p++);
	saved_line[curwin->w_cursor.col] = NUL;
    }

    if ((State & INSERT) && !(State & VREPLACE_FLAG))
    {
	p_extra = saved_line + curwin->w_cursor.col;
#ifdef FEAT_SMARTINDENT
	if (do_si)		/* need first char after new line break */
	{
	    p = skipwhite(p_extra);
	    first_char = *p;
	}
#endif
#ifdef FEAT_COMMENTS
	extra_len = (int)STRLEN(p_extra);
#endif
	saved_char = *p_extra;
	*p_extra = NUL;
    }

    u_clearline();		/* cannot do "U" command when adding lines */
#ifdef FEAT_SMARTINDENT
    did_si = FALSE;
#endif
    ai_col = 0;

    /*
     * If we just did an auto-indent, then we didn't type anything on
     * the prior line, and it should be truncated.  Do this even if 'ai' is not
     * set because automatically inserting a comment leader also sets did_ai.
     */
    if (dir == FORWARD && did_ai)
	trunc_line = TRUE;

    /*
     * If 'autoindent' and/or 'smartindent' is set, try to figure out what
     * indent to use for the new line.
     */
    if (curbuf->b_p_ai
#ifdef FEAT_SMARTINDENT
			|| do_si
#endif
					    )
    {
	/*
	 * count white space on current line
	 */
	newindent = get_indent_str(saved_line, (int)curbuf->b_p_ts);
	if (newindent == 0)
	    newindent = old_indent;	/* for ^^D command in insert mode */

#ifdef FEAT_SMARTINDENT
	/*
	 * Do smart indenting.
	 * In insert/replace mode (only when dir == FORWARD)
	 * we may move some text to the next line. If it starts with '{'
	 * don't add an indent. Fixes inserting a NL before '{' in line
	 *	"if (condition) {"
	 */
	if (!trunc_line && do_si && *saved_line != NUL
				    && (p_extra == NULL || first_char != '{'))
	{
	    char_u  *ptr;
	    char_u  last_char;

	    old_cursor = curwin->w_cursor;
	    ptr = saved_line;
#ifdef FEAT_COMMENTS
	    if (fo_do_comments)
		lead_len = get_leader_len(ptr, NULL, FALSE);
	    else
		lead_len = 0;
#endif
	    if (dir == FORWARD)
	    {
		/*
		 * Skip preprocessor directives, unless they are
		 * recognised as comments.
		 */
		if (
#ifdef FEAT_COMMENTS
			lead_len == 0 &&
#endif
			ptr[0] == '#')
		{
		    while (ptr[0] == '#' && curwin->w_cursor.lnum > 1)
			ptr = ml_get(--curwin->w_cursor.lnum);
		    newindent = get_indent();
		}
#ifdef FEAT_COMMENTS
		if (fo_do_comments)
		    lead_len = get_leader_len(ptr, NULL, FALSE);
		else
		    lead_len = 0;
		if (lead_len > 0)
		{
		    /*
		     * This case gets the following right:
		     *	    \*
		     *	     * A comment (read '\' as '/').
		     *	     *\
		     * #define IN_THE_WAY
		     *	    This should line up here;
		     */
		    p = skipwhite(ptr);
		    if (p[0] == '/' && p[1] == '*')
			p++;
		    if (p[0] == '*')
		    {
			for (p++; *p; p++)
			{
			    if (p[0] == '/' && p[-1] == '*')
			    {
				/*
				 * End of C comment, indent should line up
				 * with the line containing the start of
				 * the comment
				 */
				curwin->w_cursor.col = (colnr_T)(p - ptr);
				if ((pos = findmatch(NULL, NUL)) != NULL)
				{
				    curwin->w_cursor.lnum = pos->lnum;
				    newindent = get_indent();
				}
			    }
			}
		    }
		}
		else	/* Not a comment line */
#endif
		{
		    /* Find last non-blank in line */
		    p = ptr + STRLEN(ptr) - 1;
		    while (p > ptr && vim_iswhite(*p))
			--p;
		    last_char = *p;

		    /*
		     * find the character just before the '{' or ';'
		     */
		    if (last_char == '{' || last_char == ';')
		    {
			if (p > ptr)
			    --p;
			while (p > ptr && vim_iswhite(*p))
			    --p;
		    }
		    /*
		     * Try to catch lines that are split over multiple
		     * lines.  eg:
		     *	    if (condition &&
		     *			condition) {
		     *		Should line up here!
		     *	    }
		     */
		    if (*p == ')')
		    {
			curwin->w_cursor.col = (colnr_T)(p - ptr);
			if ((pos = findmatch(NULL, '(')) != NULL)
			{
			    curwin->w_cursor.lnum = pos->lnum;
			    newindent = get_indent();
			    ptr = ml_get_curline();
			}
		    }
		    /*
		     * If last character is '{' do indent, without
		     * checking for "if" and the like.
		     */
		    if (last_char == '{')
		    {
			did_si = TRUE;	/* do indent */
			no_si = TRUE;	/* don't delete it when '{' typed */
		    }
		    /*
		     * Look for "if" and the like, use 'cinwords'.
		     * Don't do this if the previous line ended in ';' or
		     * '}'.
		     */
		    else if (last_char != ';' && last_char != '}'
						       && cin_is_cinword(ptr))
			did_si = TRUE;
		}
	    }
	    else /* dir == BACKWARD */
	    {
		/*
		 * Skip preprocessor directives, unless they are
		 * recognised as comments.
		 */
		if (
#ifdef FEAT_COMMENTS
			lead_len == 0 &&
#endif
			ptr[0] == '#')
		{
		    int was_backslashed = FALSE;

		    while ((ptr[0] == '#' || was_backslashed) &&
			 curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count)
		    {
			if (*ptr && ptr[STRLEN(ptr) - 1] == '\\')
			    was_backslashed = TRUE;
			else
			    was_backslashed = FALSE;
			ptr = ml_get(++curwin->w_cursor.lnum);
		    }
		    if (was_backslashed)
			newindent = 0;	    /* Got to end of file */
		    else
			newindent = get_indent();
		}
		p = skipwhite(ptr);
		if (*p == '}')	    /* if line starts with '}': do indent */
		    did_si = TRUE;
		else		    /* can delete indent when '{' typed */
		    can_si_back = TRUE;
	    }
	    curwin->w_cursor = old_cursor;
	}
	if (do_si)
	    can_si = TRUE;
#endif /* FEAT_SMARTINDENT */

	did_ai = TRUE;
    }

#ifdef FEAT_COMMENTS
    /*
     * Find out if the current line starts with a comment leader.
     * This may then be inserted in front of the new line.
     */
    end_comment_pending = NUL;
    if (fo_do_comments)
	lead_len = get_leader_len(saved_line, &lead_flags, dir == BACKWARD);
    else
	lead_len = 0;
    if (lead_len > 0)
    {
	char_u	*lead_repl = NULL;	    /* replaces comment leader */
	int	lead_repl_len = 0;	    /* length of *lead_repl */
	char_u	lead_middle[COM_MAX_LEN];   /* middle-comment string */
	char_u	lead_end[COM_MAX_LEN];	    /* end-comment string */
	char_u	*comment_end = NULL;	    /* where lead_end has been found */
	int	extra_space = FALSE;	    /* append extra space */
	int	current_flag;
	int	require_blank = FALSE;	    /* requires blank after middle */
	char_u	*p2;

	/*
	 * If the comment leader has the start, middle or end flag, it may not
	 * be used or may be replaced with the middle leader.
	 */
	for (p = lead_flags; *p && *p != ':'; ++p)
	{
	    if (*p == COM_BLANK)
	    {
		require_blank = TRUE;
		continue;
	    }
	    if (*p == COM_START || *p == COM_MIDDLE)
	    {
		current_flag = *p;
		if (*p == COM_START)
		{
		    /*
		     * Doing "O" on a start of comment does not insert leader.
		     */
		    if (dir == BACKWARD)
		    {
			lead_len = 0;
			break;
		    }

		    /* find start of middle part */
		    (void)copy_option_part(&p, lead_middle, COM_MAX_LEN, ",");
		    require_blank = FALSE;
		}

		/*
		 * Isolate the strings of the middle and end leader.
		 */
		while (*p && p[-1] != ':')	/* find end of middle flags */
		{
		    if (*p == COM_BLANK)
			require_blank = TRUE;
		    ++p;
		}
		(void)copy_option_part(&p, lead_middle, COM_MAX_LEN, ",");

		while (*p && p[-1] != ':')	/* find end of end flags */
		{
		    /* Check whether we allow automatic ending of comments */
		    if (*p == COM_AUTO_END)
			end_comment_pending = -1; /* means we want to set it */
		    ++p;
		}
		n = copy_option_part(&p, lead_end, COM_MAX_LEN, ",");

		if (end_comment_pending == -1)	/* we can set it now */
		    end_comment_pending = lead_end[n - 1];

		/*
		 * If the end of the comment is in the same line, don't use
		 * the comment leader.
		 */
		if (dir == FORWARD)
		{
		    for (p = saved_line + lead_len; *p; ++p)
			if (STRNCMP(p, lead_end, n) == 0)
			{
			    comment_end = p;
			    lead_len = 0;
			    break;
			}
		}

		/*
		 * Doing "o" on a start of comment inserts the middle leader.
		 */
		if (lead_len > 0)
		{
		    if (current_flag == COM_START)
		    {
			lead_repl = lead_middle;
			lead_repl_len = (int)STRLEN(lead_middle);
		    }

		    /*
		     * If we have hit RETURN immediately after the start
		     * comment leader, then put a space after the middle
		     * comment leader on the next line.
		     */
		    if (!vim_iswhite(saved_line[lead_len - 1])
			    && ((p_extra != NULL
				    && (int)curwin->w_cursor.col == lead_len)
				|| (p_extra == NULL
				    && saved_line[lead_len] == NUL)
				|| require_blank))
			extra_space = TRUE;
		}
		break;
	    }
	    if (*p == COM_END)
	    {
		/*
		 * Doing "o" on the end of a comment does not insert leader.
		 * Remember where the end is, might want to use it to find the
		 * start (for C-comments).
		 */
		if (dir == FORWARD)
		{
		    comment_end = skipwhite(saved_line);
		    lead_len = 0;
		    break;
		}

		/*
		 * Doing "O" on the end of a comment inserts the middle leader.
		 * Find the string for the middle leader, searching backwards.
		 */
		while (p > curbuf->b_p_com && *p != ',')
		    --p;
		for (lead_repl = p; lead_repl > curbuf->b_p_com
					 && lead_repl[-1] != ':'; --lead_repl)
		    ;
		lead_repl_len = (int)(p - lead_repl);

		/* We can probably always add an extra space when doing "O" on
		 * the comment-end */
		extra_space = TRUE;

		/* Check whether we allow automatic ending of comments */
		for (p2 = p; *p2 && *p2 != ':'; p2++)
		{
		    if (*p2 == COM_AUTO_END)
			end_comment_pending = -1; /* means we want to set it */
		}
		if (end_comment_pending == -1)
		{
		    /* Find last character in end-comment string */
		    while (*p2 && *p2 != ',')
			p2++;
		    end_comment_pending = p2[-1];
		}
		break;
	    }
	    if (*p == COM_FIRST)
	    {
		/*
		 * Comment leader for first line only:	Don't repeat leader
		 * when using "O", blank out leader when using "o".
		 */
		if (dir == BACKWARD)
		    lead_len = 0;
		else
		{
		    lead_repl = (char_u *)"";
		    lead_repl_len = 0;
		}
		break;
	    }
	}
	if (lead_len)
	{
	    /* allocate buffer (may concatenate p_exta later) */
	    leader = alloc(lead_len + lead_repl_len + extra_space +
							      extra_len + 1);
	    allocated = leader;		    /* remember to free it later */

	    if (leader == NULL)
		lead_len = 0;
	    else
	    {
		STRNCPY(leader, saved_line, lead_len);
		leader[lead_len] = NUL;

		/*
		 * Replace leader with lead_repl, right or left adjusted
		 */
		if (lead_repl != NULL)
		{
		    int		c = 0;
		    int		off = 0;

		    for (p = lead_flags; *p && *p != ':'; ++p)
		    {
			if (*p == COM_RIGHT || *p == COM_LEFT)
			    c = *p;
			else if (isdigit(*p) || *p == '-')
			    off = getdigits(&p);
		    }
		    if (c == COM_RIGHT)    /* right adjusted leader */
		    {
			/* find last non-white in the leader to line up with */
			for (p = leader + lead_len - 1; p > leader
						      && vim_iswhite(*p); --p)
			    ;

			++p;
			if (p < leader + lead_repl_len)
			    p = leader;
			else
			    p -= lead_repl_len;
			mch_memmove(p, lead_repl, (size_t)lead_repl_len);
			if (p + lead_repl_len > leader + lead_len)
			    p[lead_repl_len] = NUL;

			/* blank-out any other chars from the old leader. */
			while (--p >= leader)
			    if (!vim_iswhite(*p))
				*p = ' ';
		    }
		    else		    /* left adjusted leader */
		    {
			p = skipwhite(leader);
			mch_memmove(p, lead_repl, (size_t)lead_repl_len);

			/* Replace any remaining non-white chars in the old
			 * leader by spaces.  Keep Tabs, the indent must
			 * remain the same. */
			for (p += lead_repl_len; p < leader + lead_len; ++p)
			    if (!vim_iswhite(*p))
			    {
				/* Don't put a space before a TAB. */
				if (p + 1 < leader + lead_len && p[1] == TAB)
				{
				    --lead_len;
				    mch_memmove(p, p + 1,
						     (leader + lead_len) - p);
				}
				else
				    *p = ' ';
			    }
			*p = NUL;
		    }

		    /* Recompute the indent, it may have changed. */
		    if (curbuf->b_p_ai
#ifdef FEAT_SMARTINDENT
					|| do_si
#endif
							   )
			newindent = get_indent_str(leader, (int)curbuf->b_p_ts);

		    /* Add the indent offset */
		    if (newindent + off < 0)
		    {
			off = -newindent;
			newindent = 0;
		    }
		    else
			newindent += off;

		    /* Correct trailing spaces for the shift, so that
		     * alignment remains equal. */
		    while (off > 0 && lead_len > 0
					       && leader[lead_len - 1] == ' ')
		    {
			/* Don't do it when there is a tab before the space */
			if (vim_strchr(skipwhite(leader), '\t') != NULL)
			    break;
			--lead_len;
			--off;
		    }

		    /* If the leader ends in white space, don't add an
		     * extra space */
		    if (lead_len > 0 && vim_iswhite(leader[lead_len - 1]))
			extra_space = FALSE;
		    leader[lead_len] = NUL;
		}

		if (extra_space)
		{
		    leader[lead_len++] = ' ';
		    leader[lead_len] = NUL;
		}

		newcol = lead_len;

		/*
		 * if a new indent will be set below, remove the indent that
		 * is in the comment leader
		 */
		if (newindent
#ifdef FEAT_SMARTINDENT
				|| did_si
#endif
					   )
		{
		    while (lead_len && vim_iswhite(*leader))
		    {
			--lead_len;
			--newcol;
			++leader;
		    }
		}

	    }
#ifdef FEAT_SMARTINDENT
	    did_si = can_si = FALSE;
#endif
	}
	else if (comment_end != NULL)
	{
	    /*
	     * We have finished a comment, so we don't use the leader.
	     * If this was a C-comment and 'ai' or 'si' is set do a normal
	     * indent to align with the line containing the start of the
	     * comment.
	     */
	    if (comment_end[0] == '*' && comment_end[1] == '/' &&
			(curbuf->b_p_ai
#ifdef FEAT_SMARTINDENT
					|| do_si
#endif
							   ))
	    {
		old_cursor = curwin->w_cursor;
		curwin->w_cursor.col = (colnr_T)(comment_end - saved_line);
		if ((pos = findmatch(NULL, NUL)) != NULL)
		{
		    curwin->w_cursor.lnum = pos->lnum;
		    newindent = get_indent();
		}
		curwin->w_cursor = old_cursor;
	    }
	}
    }
#endif

    /* (State == INSERT || State == REPLACE), only when dir == FORWARD */
    if (p_extra != NULL)
    {
	*p_extra = saved_char;		/* restore char that NUL replaced */

	/*
	 * When 'ai' set or "del_spaces" TRUE, skip to the first non-blank.
	 *
	 * When in REPLACE mode, put the deleted blanks on the replace stack,
	 * preceded by a NUL, so they can be put back when a BS is entered.
	 */
	if ((State & REPLACE_FLAG) && !(State & VREPLACE_FLAG))
	    replace_push(NUL);	    /* end of extra blanks */
	if (curbuf->b_p_ai || del_spaces)
	{
	    while (*p_extra == ' ' || *p_extra == '\t')
	    {
		if ((State & REPLACE_FLAG) && !(State & VREPLACE_FLAG))
		    replace_push(*p_extra);
		++p_extra;
	    }
	}
	if (*p_extra != NUL)
	    did_ai = FALSE;	    /* append some text, don't truncate now */
    }

    if (p_extra == NULL)
	p_extra = (char_u *)"";		    /* append empty line */

#ifdef FEAT_COMMENTS
    /* concatenate leader and p_extra, if there is a leader */
    if (lead_len)
    {
	STRCAT(leader, p_extra);
	p_extra = leader;
	did_ai = TRUE;	    /* So truncating blanks works with comments */
    }
    else
	end_comment_pending = NUL;  /* turns out there was no leader */
#endif

    old_cursor = curwin->w_cursor;
    if (dir == BACKWARD)
	--curwin->w_cursor.lnum;
    if (!(State & VREPLACE_FLAG) || old_cursor.lnum >= orig_line_count)
    {
	if (ml_append(curwin->w_cursor.lnum, p_extra, (colnr_T)0, FALSE)
								      == FAIL)
	    goto theend;
	/* Postpone calling changed_lines(), because it would mess up folding
	 * with markers. */
	mark_adjust(curwin->w_cursor.lnum + 1, (linenr_T)MAXLNUM, 1L, 0L);
	did_append = TRUE;
    }
    else
    {
	/*
	 * In VREPLACE mode we are starting to replace the next line.
	 */
	curwin->w_cursor.lnum++;
	if (curwin->w_cursor.lnum >= Insstart.lnum + vr_lines_changed)
	{
	    /* In case we NL to a new line, BS to the previous one, and NL
	     * again, we don't want to save the new line for undo twice.
	     */
	    (void)u_save_cursor();		    /* errors are ignored! */
	    vr_lines_changed++;
	}
	ml_replace(curwin->w_cursor.lnum, p_extra, TRUE);
	changed_bytes(curwin->w_cursor.lnum, 0);
	curwin->w_cursor.lnum--;
	did_append = FALSE;
    }

    if (newindent
#ifdef FEAT_SMARTINDENT
		    || did_si
#endif
				)
    {
	++curwin->w_cursor.lnum;
#ifdef FEAT_SMARTINDENT
	if (did_si)
	{
	    if (p_sr)
		newindent -= newindent % (int)curbuf->b_p_sw;
	    newindent += (int)curbuf->b_p_sw;
	}
#endif
	(void)set_indent(newindent, SIN_INSERT);
	ai_col = curwin->w_cursor.col;

	/*
	 * In REPLACE mode, for each character in the new indent, there must
	 * be a NUL on the replace stack, for when it is deleted with BS
	 */
	if ((State & REPLACE_FLAG) && !(State & VREPLACE_FLAG))
	    for (n = 0; n < (int)curwin->w_cursor.col; ++n)
		replace_push(NUL);
	newcol += curwin->w_cursor.col;
#ifdef FEAT_SMARTINDENT
	if (no_si)
	    did_si = FALSE;
#endif
    }

#ifdef FEAT_COMMENTS
    /*
     * In REPLACE mode, for each character in the extra leader, there must be
     * a NUL on the replace stack, for when it is deleted with BS.
     */
    if ((State & REPLACE_FLAG) && !(State & VREPLACE_FLAG))
	while (lead_len-- > 0)
	    replace_push(NUL);
#endif

    curwin->w_cursor = old_cursor;

    if (dir == FORWARD)
    {
	if (trunc_line || (State & INSERT))
	{
	    /* truncate current line at cursor */
	    saved_line[curwin->w_cursor.col] = NUL;
	    if (trunc_line)	    /* Remove trailing white space */
		truncate_spaces(saved_line);
	    ml_replace(curwin->w_cursor.lnum, saved_line, FALSE);
	    saved_line = NULL;
	    if (did_append)
	    {
		changed_lines(curwin->w_cursor.lnum, curwin->w_cursor.col,
					       curwin->w_cursor.lnum + 1, 1L);
		did_append = FALSE;
	    }
	    else
		changed_bytes(curwin->w_cursor.lnum, curwin->w_cursor.col);
	}

	/*
	 * Put the cursor on the new line.  Careful: the scrollup() above may
	 * have moved w_cursor, we must use old_cursor.
	 */
	curwin->w_cursor.lnum = old_cursor.lnum + 1;
    }
    if (did_append)
	changed_lines(curwin->w_cursor.lnum, 0, curwin->w_cursor.lnum, 1L);

    curwin->w_cursor.col = newcol;
#ifdef FEAT_VIRTUALEDIT
    curwin->w_cursor.coladd = 0;
#endif

#if defined(FEAT_LISP) || defined(FEAT_CINDENT)
    /*
     * In VREPLACE mode, we are handling the replace stack ourselves, so stop
     * fixthisline() from doing it (via change_indent()) by telling it we're in
     * normal INSERT mode.
     */
    if (State & VREPLACE_FLAG)
    {
	vreplace_mode = State;	/* So we know to put things right later */
	State = INSERT;
    }
    else
	vreplace_mode = 0;
#endif
#ifdef FEAT_LISP
    /*
     * May do lisp indenting.
     */
    if (!p_paste
# ifdef FEAT_COMMENTS
	    && leader == NULL
# endif
	    && curbuf->b_p_lisp
	    && curbuf->b_p_ai)
    {
	fixthisline(get_lisp_indent);
	p = ml_get_curline();
	ai_col = (colnr_T)(skipwhite(p) - p);
    }
#endif
#ifdef FEAT_CINDENT
    /*
     * May do indenting after opening a new line.
     */
    if (!p_paste
# ifdef FEAT_COMMENTS
	    && (leader == NULL || !curbuf->b_p_ai)
# endif
	    && (curbuf->b_p_cin
#  ifdef FEAT_EVAL
		    || *curbuf->b_p_inde != NUL
#  endif
		)
	    && in_cinkeys(dir == FORWARD
		? KEY_OPEN_FORW
		: KEY_OPEN_BACK, ' ', linewhite(curwin->w_cursor.lnum)))
    {
	do_c_expr_indent();
	p = ml_get_curline();
	ai_col = (colnr_T)(skipwhite(p) - p);
    }
#endif
#if defined(FEAT_LISP) || defined(FEAT_CINDENT)
    if (vreplace_mode != 0)
	State = vreplace_mode;
#endif

    /*
     * Finally, VREPLACE gets the stuff on the new line, then puts back the
     * original line, and inserts the new stuff char by char, pushing old stuff
     * onto the replace stack (via ins_char()).
     */
    if (State & VREPLACE_FLAG)
    {
	/* Put new line in p_extra */
	p_extra = vim_strsave(ml_get_curline());
	if (p_extra == NULL)
	    goto theend;

	/* Put back original line */
	ml_replace(curwin->w_cursor.lnum, next_line, FALSE);

	/* Insert new stuff into line again */
	curwin->w_cursor.col = 0;
#ifdef FEAT_VIRTUALEDIT
	curwin->w_cursor.coladd = 0;
#endif
	vr_virtcol = MAXCOL;
	ins_bytes(p_extra);	/* will call changed_bytes() */
	next_line = NULL;
    }

    retval = TRUE;		/* success! */
theend:
    vim_free(saved_line);
    vim_free(next_line);
    vim_free(allocated);
    return retval;
}

#if defined(FEAT_COMMENTS) || defined(PROTO)
/*
 * get_leader_len() returns the length of the prefix of the given string
 * which introduces a comment.	If this string is not a comment then 0 is
 * returned.
 * When "flags" is not NULL, it is set to point to the flags of the recognized
 * comment leader.
 * "backward" must be true for the "O" command.
 */
    int
get_leader_len(line, flags, backward)
    char_u	*line;
    char_u	**flags;
    int		backward;
{
    int		i, j;
    int		got_com = FALSE;
    int		found_one;
    char_u	part_buf[COM_MAX_LEN];	/* buffer for one option part */
    char_u	*string;		/* pointer to comment string */
    char_u	*list;

    i = 0;
    while (vim_iswhite(line[i]))    /* leading white space is ignored */
	++i;

    /*
     * Repeat to match several nested comment strings.
     */
    while (line[i])
    {
	/*
	 * scan through the 'comments' option for a match
	 */
	found_one = FALSE;
	for (list = curbuf->b_p_com; *list; )
	{
	    /*
	     * Get one option part into part_buf[].  Advance list to next one.
	     * put string at start of string.
	     */
	    if (!got_com && flags != NULL)  /* remember where flags started */
		*flags = list;
	    (void)copy_option_part(&list, part_buf, COM_MAX_LEN, ",");
	    string = vim_strchr(part_buf, ':');
	    if (string == NULL)	    /* missing ':', ignore this part */
		continue;
	    *string++ = NUL;	    /* isolate flags from string */

	    /*
	     * When already found a nested comment, only accept further
	     * nested comments.
	     */
	    if (got_com && vim_strchr(part_buf, COM_NEST) == NULL)
		continue;

	    /* When 'O' flag used don't use for "O" command */
	    if (backward && vim_strchr(part_buf, COM_NOBACK) != NULL)
		continue;

	    /*
	     * Line contents and string must match.
	     * When string starts with white space, must have some white space
	     * (but the amount does not need to match, there might be a mix of
	     * TABs and spaces).
	     */
	    if (vim_iswhite(string[0]))
	    {
		if (i == 0 || !vim_iswhite(line[i - 1]))
		    continue;
		while (vim_iswhite(string[0]))
		    ++string;
	    }
	    for (j = 0; string[j] != NUL && string[j] == line[i + j]; ++j)
		;
	    if (string[j] != NUL)
		continue;

	    /*
	     * When 'b' flag used, there must be white space or an
	     * end-of-line after the string in the line.
	     */
	    if (vim_strchr(part_buf, COM_BLANK) != NULL
			   && !vim_iswhite(line[i + j]) && line[i + j] != NUL)
		continue;

	    /*
	     * We have found a match, stop searching.
	     */
	    i += j;
	    got_com = TRUE;
	    found_one = TRUE;
	    break;
	}

	/*
	 * No match found, stop scanning.
	 */
	if (!found_one)
	    break;

	/*
	 * Include any trailing white space.
	 */
	while (vim_iswhite(line[i]))
	    ++i;

	/*
	 * If this comment doesn't nest, stop here.
	 */
	if (vim_strchr(part_buf, COM_NEST) == NULL)
	    break;
    }
    return (got_com ? i : 0);
}
#endif

/*
 * Return the number of window lines occupied by buffer line "lnum".
 */
    int
plines(lnum)
    linenr_T	lnum;
{
    return plines_win(curwin, lnum, TRUE);
}

    int
plines_win(wp, lnum, winheight)
    win_T	*wp;
    linenr_T	lnum;
    int		winheight;	/* when TRUE limit to window height */
{
#if defined(FEAT_DIFF) || defined(PROTO)
    /* Check for filler lines above this buffer line.  When folded the result
     * is one line anyway. */
    return plines_win_nofill(wp, lnum, winheight) + diff_check_fill(wp, lnum);
}

    int
plines_nofill(lnum)
    linenr_T	lnum;
{
    return plines_win_nofill(curwin, lnum, TRUE);
}

    int
plines_win_nofill(wp, lnum, winheight)
    win_T	*wp;
    linenr_T	lnum;
    int		winheight;	/* when TRUE limit to window height */
{
#endif
    int		lines;

    if (!wp->w_p_wrap)
	return 1;

#ifdef FEAT_VERTSPLIT
    if (wp->w_width == 0)
	return 1;
#endif

#ifdef FEAT_FOLDING
    /* A folded lines is handled just like an empty line. */
    /* NOTE: Caller must handle lines that are MAYBE folded. */
    if (lineFolded(wp, lnum) == TRUE)
	return 1;
#endif

    lines = plines_win_nofold(wp, lnum);
    if (winheight > 0 && lines > wp->w_height)
	return (int)wp->w_height;
    return lines;
}

/*
 * Return number of window lines physical line "lnum" will occupy in window
 * "wp".  Does not care about folding, 'wrap' or 'diff'.
 */
    int
plines_win_nofold(wp, lnum)
    win_T	*wp;
    linenr_T	lnum;
{
    char_u	*s;
    long	col;
    int		width;

    s = ml_get_buf(wp->w_buffer, lnum, FALSE);
    if (*s == NUL)		/* empty line */
	return 1;
    col = win_linetabsize(wp, s, (colnr_T)MAXCOL);

    /*
     * If list mode is on, then the '$' at the end of the line may take up one
     * extra column.
     */
    if (wp->w_p_list && lcs_eol != NUL)
	col += 1;

    /*
     * Add column offset for 'number' and 'foldcolumn'.
     */
    width = W_WIDTH(wp) - win_col_off(wp);
    if (width <= 0)
	return 32000;
    if (col <= width)
	return 1;
    col -= width;
    width += win_col_off2(wp);
    return (col + (width - 1)) / width + 1;
}

/*
 * Like plines_win(), but only reports the number of physical screen lines
 * used from the start of the line to the given column number.
 */
    int
plines_win_col(wp, lnum, column)
    win_T	*wp;
    linenr_T	lnum;
    long	column;
{
    long	col;
    char_u	*s;
    int		lines = 0;
    int		width;

#ifdef FEAT_DIFF
    /* Check for filler lines above this buffer line.  When folded the result
     * is one line anyway. */
    lines = diff_check_fill(wp, lnum);
#endif

    if (!wp->w_p_wrap)
	return lines + 1;

#ifdef FEAT_VERTSPLIT
    if (wp->w_width == 0)
	return lines + 1;
#endif

    s = ml_get_buf(wp->w_buffer, lnum, FALSE);

    col = 0;
    while (*s != NUL && --column >= 0)
    {
	col += win_lbr_chartabsize(wp, s, (colnr_T)col, NULL);
#ifdef FEAT_MBYTE
	if (has_mbyte)
	    s += (*mb_ptr2len_check)(s);
	else
#endif
	    ++s;
    }

    /*
     * If *s is a TAB, and the TAB is not displayed as ^I, and we're not in
     * INSERT mode, then col must be adjusted so that it represents the last
     * screen position of the TAB.  This only fixes an error when the TAB wraps
     * from one screen line to the next (when 'columns' is not a multiple of
     * 'ts') -- webb.
     */
    if (*s == TAB && (State & NORMAL) && (!wp->w_p_list || lcs_tab1))
	col += win_lbr_chartabsize(wp, s, (colnr_T)col, NULL) - 1;

    /*
     * Add column offset for 'number', 'foldcolumn', etc.
     */
    width = W_WIDTH(wp) - win_col_off(wp);
    if (width > 0)
    {
	lines += 1;
	if (col >= width)
	    lines += (col - width) / (width + win_col_off2(wp));
	if (lines <= wp->w_height)
	    return lines;
    }
    return (int)(wp->w_height);	    /* maximum length */
}

    int
plines_m_win(wp, first, last)
    win_T	*wp;
    linenr_T	first, last;
{
    int		count = 0;

    while (first <= last)
    {
#ifdef FEAT_FOLDING
	int	x;

	/* Check if there are any really folded lines, but also included lines
	 * that are maybe folded. */
	x = foldedCount(wp, first, NULL);
	if (x > 0)
	{
	    ++count;	    /* count 1 for "+-- folded" line */
	    first += x;
	}
	else
#endif
	{
#ifdef FEAT_DIFF
	    if (first == wp->w_topline)
		count += plines_win_nofill(wp, first, TRUE) + wp->w_topfill;
	    else
#endif
		count += plines_win(wp, first, TRUE);
	    ++first;
	}
    }
    return (count);
}

/*
 * Disable 'list' temporarily, unless 'cpo' contains the 'L' flag.
 * Returns the old value of list, so when finished, curwin->w_p_list should be
 * set back to this.
 */
    static int
temporary_nolist()
{
    int	    old_list = curwin->w_p_list;

    if (old_list && vim_strchr(p_cpo, CPO_LISTWM) == NULL)
	curwin->w_p_list = FALSE;
    return old_list;
}

/*
 * Insert string "p" at the cursor position.  Stops at a NUL byte.
 * Handles Replace mode and multi-byte characters.
 */
    void
ins_bytes(p)
    char_u	*p;
{
    ins_bytes_len(p, (int)STRLEN(p));
}

/*
 * Insert string "p" with length "len" at the cursor position.
 * Handles Replace mode and multi-byte characters.
 */
    void
ins_bytes_len(p, len)
    char_u	*p;
    int		len;
{
    int		i;
#ifdef FEAT_MBYTE
    int		n;

    for (i = 0; i < len; i += n)
    {
	n = (*mb_ptr2len_check)(p + i);
	ins_char_bytes(p + i, n);
    }
#else
    for (i = 0; i < len; ++i)
	ins_char(p[i]);
#endif
}

/*
 * Insert or replace a single character at the cursor position.
 * When in REPLACE or VREPLACE mode, replace any existing character.
 * Caller must have prepared for undo.
 * For multi-byte characters we get the whole character, the caller must
 * convert bytes to a character.
 */
    void
ins_char(c)
    int		c;
{
#if defined(FEAT_MBYTE) || defined(PROTO)
    char_u	buf[MB_MAXBYTES];
    int		n;

    n = (*mb_char2bytes)(c, buf);
    ins_char_bytes(buf, n);
}

    void
ins_char_bytes(buf, newlen)
    char_u	*buf;
    int		newlen;
{
    int		c = buf[0];
    int		oldlen;
#endif
    char_u	*p;
    char_u	*newp;
    char_u	*oldp;
    int		linelen;	/* length of old line including NUL */
    int		extra;		/* number of extra bytes (can be negative) */
    colnr_T	col;
    linenr_T	lnum = curwin->w_cursor.lnum;
    int		vcol;
    int		new_vcol = 0;   /* init for GCC */
    int		i;
    int		size;
    int		old_list;

#ifdef FEAT_VIRTUALEDIT
    /* Break tabs if needed. */
    if (virtual_active())
    {
	p = ml_get_cursor();
	if (*p == NUL
		|| (*p == TAB && (chartabsize(p, curwin->w_cursor.col) > 1)))
	    coladvance_force(getviscol());
    }
#endif

    col = curwin->w_cursor.col;
    oldp = ml_get(lnum);
    linelen = (int)STRLEN(oldp) + 1;

    if ((State & REPLACE_FLAG) && !(State & VREPLACE_FLAG) && oldp[col] != NUL)
    {
#ifdef FEAT_MBYTE
	if (has_mbyte)
	    oldlen = (*mb_ptr2len_check)(oldp + col);
	else
	    oldlen = 1;
#else
	extra = 0;			/* overwriting */
#endif
    }
    else
#ifdef FEAT_MBYTE
	oldlen = 0;
#else
	extra = 1;			/* inserting or appending */
#endif
#ifdef FEAT_MBYTE
    extra = newlen - oldlen;	/* extra can be negative! */
#endif

    /*
     * A character has to be put on the replace stack if there is a
     * character that is replaced, so it can be put back when BS is used.
     */
    if ((State & REPLACE_FLAG) && !(State & VREPLACE_FLAG))
    {
	replace_push(NUL);
	if (oldp[col] != NUL)
	{
#ifdef FEAT_MBYTE
	    for (i = oldlen - 1; i >= 0; --i)
		replace_push(oldp[col + i]);
#else
	    replace_push(oldp[col]);
#endif
	}
    }

    /*
     * In virtual replace mode each character may replace any number of
     * characters from zero up.  Make sure BS works.
     */
    if (State & VREPLACE_FLAG)
    {
#ifndef FEAT_MBYTE
	char_u	buf[2];
#endif

	old_list = temporary_nolist();
	if (vr_virtcol == MAXCOL)
	{
	    getvcol(curwin, &curwin->w_cursor, NULL, &vr_virtcol, NULL);

	    /* In case we backspace over one of the two normal characters that
	     * replaced a single CTRL character, and then typed a normal
	     * character again.  We need to know whether to replace the CTRL
	     * character or not.
	     */
	    vr_virtoffset = vr_virtcol - get_replace_stack_virtcol();
	}
	vcol = vr_virtcol;
	replace_push(NUL);
#ifndef FEAT_MBYTE
	buf[0] = c;
	buf[1] = NUL;
#endif
	new_vcol = vcol + chartabsize(buf, vcol);
	vcol -= vr_virtoffset;
	vr_virtoffset = 0;
	i = col;
	while (oldp[i] != NUL)
	{
#ifdef FEAT_MBYTE
	    int byte_length, j;
#endif

	    size = chartabsize(oldp + i, vcol);
	    vcol += size;
	    if (vcol > new_vcol)
	    {
		/* Check for doing something like typing "a" over "^A".  Can't
		 * keep columns aligned, so set vr_virtoffset to 1.
		 */
		if (size == 2 && (oldp[i] != TAB || curwin->w_p_list)
						&& vcol == new_vcol + 1)
		    vr_virtoffset = 1;
		break;
	    }
#ifdef FEAT_MBYTE
	    /* Push (multibyte) character to replace stack (last byte first) */
	    byte_length = (*mb_ptr2len_check)(oldp + i);
	    for (j = byte_length - 1; j >= 0; --j)
		replace_push(oldp[i + j]);
	    i += byte_length;
	    extra -= byte_length;
#else
	    replace_push(oldp[i++]);
	    extra--;	/* Goes negative when replacing more than one char */
#endif
	}
#ifdef FEAT_MBYTE
	oldlen = i - col;
#endif
	curwin->w_p_list = old_list;
    }

    newp = alloc_check((unsigned)(linelen + extra));
    if (newp == NULL)
	return;

    /* Copy bytes before the cursor. */
    if (col > 0)
	mch_memmove(newp, oldp, (size_t)col);

    /* Copy bytes after (and under) the cursor. */
    p = newp + col;
    if ((State & VREPLACE_FLAG) && extra <= 0)
    {
	i = col - extra + 1;
	mch_memmove(p + 1, oldp + i, (size_t)(linelen - i));
    }
    else
#ifdef FEAT_MBYTE
	mch_memmove(p + newlen, oldp + col + oldlen,
					    (size_t)(linelen - col - oldlen));
#else
	mch_memmove(p + extra, oldp + col, (size_t)(linelen - col));
#endif

    /* Insert or overwrite the new character. */
#ifdef FEAT_MBYTE
    mch_memmove(p, buf, newlen);
#else
    *p = c;
#endif

    /* Replace the line in the buffer. */
    ml_replace(lnum, newp, FALSE);

    /* mark the buffer as changed and prepare for displaying */
    changed_bytes(lnum, col);

    /*
     * If we're in Insert or Replace mode and 'showmatch' is set, then briefly
     * show the match for right parens and braces.
     */
    if (p_sm && (State & INSERT)
	    && msg_silent == 0
#ifdef FEAT_MBYTE
	    && newlen == 1
#endif
#ifdef FEAT_RIGHTLEFT
	    && ((!(curwin->w_p_rl ^ p_ri)
		    && (c == ')' || c == '}' || c == ']'))
		|| ((curwin->w_p_rl ^ p_ri)
		    && (c == '(' || c == '{' || c == '[')))
#else
	    && (c == ')' || c == '}' || c == ']')
#endif
	    )
	showmatch();

#ifdef FEAT_RIGHTLEFT
    if (!p_ri || (State & REPLACE_FLAG))
#endif
    {
	/* Normal insert: move cursor right */
#ifdef FEAT_MBYTE
	curwin->w_cursor.col += newlen;
#else
	++curwin->w_cursor.col;
#endif

	/* For VREPLACE mode */
	vr_virtcol = new_vcol;
    }
    /*
     * TODO: should try to update w_row here, to avoid recomputing it later.
     */
}

/*
 * Insert a string at the cursor position.
 * Note: Does NOT handle Replace mode.
 * Caller must have prepared for undo.
 */
    void
ins_str(s)
    char_u	*s;
{
    char_u	*oldp, *newp;
    int		newlen = (int)STRLEN(s);
    int		oldlen;
    colnr_T	col;
    linenr_T	lnum = curwin->w_cursor.lnum;

#ifdef FEAT_VIRTUALEDIT
    if (virtual_active() && curwin->w_cursor.coladd)
	coladvance_force(getviscol());
#endif

    col = curwin->w_cursor.col;
    oldp = ml_get(lnum);
    oldlen = (int)STRLEN(oldp);

    newp = alloc_check((unsigned)(oldlen + newlen + 1));
    if (newp == NULL)
	return;
    if (col > 0)
	mch_memmove(newp, oldp, (size_t)col);
    mch_memmove(newp + col, s, (size_t)newlen);
    mch_memmove(newp + col + newlen, oldp + col, (size_t)(oldlen - col + 1));
    ml_replace(lnum, newp, FALSE);
    changed_bytes(lnum, col);
    curwin->w_cursor.col += newlen;
}

/*
 * Delete one character under the cursor.
 * If 'fixpos' is TRUE, don't leave the cursor on the NUL after the line.
 * Caller must have prepared for undo.
 *
 * return FAIL for failure, OK otherwise
 */
    int
del_char(fixpos)
    int		fixpos;
{
#ifdef FEAT_MBYTE
    char_u	*p;

    if (has_mbyte)
    {
	/* delete multi-bytes, when the cursor is on a multi-byte character */
	mb_adjust_cursor();

	p = ml_get_cursor();
	if (p == NULL || p[0] == NUL)
	    return FALSE;

	return del_bytes((long)(*mb_ptr2len_check)(p), fixpos);
    }
#endif
    return del_bytes(1L, fixpos);
}

/*
 * Delete 'count' bytes under the cursor.
 * If 'fixpos' is TRUE, don't leave the cursor on the NUL after the line.
 * Caller must have prepared for undo.
 *
 * return FAIL for failure, OK otherwise
 */
    int
del_bytes(count, fixpos)
    long	count;
    int		fixpos;
{
    char_u	*oldp, *newp;
    colnr_T	oldlen;
    linenr_T	lnum = curwin->w_cursor.lnum;
    colnr_T	col = curwin->w_cursor.col;
    int		was_alloced;
    long	movelen;
#ifdef FEAT_MBYTE
    int		p0, p1, p2;
    int		p0len, p1len, p2len;
#endif

    oldp = ml_get(lnum);
    oldlen = (int)STRLEN(oldp);

    /*
     * Can't do anything when the cursor is on the NUL after the line.
     */
    if (col >= oldlen)
	return FAIL;

#ifdef FEAT_MBYTE
    if (p_deco)
    {
	p1 = p2 = 0;
	/* see if there are any combining characters: */
	p0 = utfc_ptr2char(oldp + col, &p1, &p2);
	p0len = utf_char2len(p0);
	p1len = p1 != 0 ? utf_char2len(p1) : 0;
	p2len = p2 != 0 ? utf_char2len(p2) : 0;

	/* don't try anything if trying to del more than one 'char' */
	if ((count <= (p0len + p1len + p2len)) && p1 != 0)
	{
	    /* We are here because there are combining characters; either
	     * p1, p2 or both.  We need to remove just that character, and
	     * leave the cursor where it was. */

	    /* since p1 is always valid, adjust for it */
	    col += p0len;
	    count = p1len;
	    if (p2 != 0)
	    {
		/* p2 is valid, so remove it instead of p1 */
		col += p1len;
		count = p2len;
	    }
	    fixpos = 0;
	}
    }
#endif

    /*
     * When count is too big, reduce it.
     */
    movelen = (long)oldlen - (long)col - count + 1; /* includes trailing NUL */
    if (movelen <= 1)
    {
	/*
	 * If we just took off the last character of a non-blank line, and
	 * fixpos is TRUE, we don't want to end up positioned at the NUL,
	 * unless we are in virtual mode.
	 */
	if (col > 0 && fixpos && !virtual_active())
	{
	    --curwin->w_cursor.col;
#ifdef FEAT_MBYTE
	    if (has_mbyte)
		curwin->w_cursor.col -=
			    (*mb_head_off)(oldp, oldp + curwin->w_cursor.col);
#endif
	}
	count = oldlen - col;
	movelen = 1;
    }

    /*
     * If the old line has been allocated the deletion can be done in the
     * existing line. Otherwise a new line has to be allocated
     */
    was_alloced = ml_line_alloced();	    /* check if oldp was allocated */
    if (was_alloced)
	newp = oldp;			    /* use same allocated memory */
    else
    {					    /* need to allocated a new line */
	newp = alloc((unsigned)(oldlen + 1 - count));
	if (newp == NULL)
	    return FAIL;
	mch_memmove(newp, oldp, (size_t)col);
    }
    mch_memmove(newp + col, oldp + col + count, (size_t)movelen);
    if (!was_alloced)
	ml_replace(lnum, newp, FALSE);

    /* mark the buffer as changed and prepare for displaying */
    changed_bytes(lnum, curwin->w_cursor.col);

    return OK;
}

/*
 * Delete from cursor to end of line.
 * Caller must have prepared for undo.
 *
 * return FAIL for failure, OK otherwise
 */
    int
truncate_line(fixpos)
    int		fixpos;	    /* if TRUE fix the cursor position when done */
{
    char_u	*newp;
    linenr_T	lnum = curwin->w_cursor.lnum;
    colnr_T	col = curwin->w_cursor.col;

    if (col == 0)
	newp = vim_strsave((char_u *)"");
    else
	newp = vim_strnsave(ml_get(lnum), col);

    if (newp == NULL)
	return FAIL;

    ml_replace(lnum, newp, FALSE);

    /* mark the buffer as changed and prepare for displaying */
    changed_bytes(lnum, curwin->w_cursor.col);

    /*
     * If "fixpos" is TRUE we don't want to end up positioned at the NUL.
     */
    if (fixpos && curwin->w_cursor.col > 0)
	--curwin->w_cursor.col;

    return OK;
}

/*
 * Delete "nlines" lines at the cursor.
 * Saves the lines for undo first if "undo" is TRUE.
 */
    void
del_lines(nlines, undo)
    long	nlines;		/* number of lines to delete */
    int		undo;		/* if TRUE, prepare for undo */
{
    long	n;

    if (nlines <= 0)
	return;

    /* save the deleted lines for undo */
    if (undo && u_savedel(curwin->w_cursor.lnum, nlines) == FAIL)
	return;

    for (n = 0; n < nlines; )
    {
	if (curbuf->b_ml.ml_flags & ML_EMPTY)	    /* nothing to delete */
	    break;

	ml_delete(curwin->w_cursor.lnum, TRUE);
	++n;

	/* If we delete the last line in the file, stop */
	if (curwin->w_cursor.lnum > curbuf->b_ml.ml_line_count)
	    break;
    }
    /* adjust marks, mark the buffer as changed and prepare for displaying */
    deleted_lines_mark(curwin->w_cursor.lnum, n);

    curwin->w_cursor.col = 0;
    check_cursor_lnum();
}

    int
gchar_pos(pos)
    pos_T *pos;
{
    char_u	*ptr = ml_get_pos(pos);

#ifdef FEAT_MBYTE
    if (has_mbyte)
	return (*mb_ptr2char)(ptr);
#endif
    return (int)*ptr;
}

    int
gchar_cursor()
{
#ifdef FEAT_MBYTE
    if (has_mbyte)
	return (*mb_ptr2char)(ml_get_cursor());
#endif
    return (int)*ml_get_cursor();
}

/*
 * Write a character at the current cursor position.
 * It is directly written into the block.
 */
    void
pchar_cursor(c)
    int c;
{
    *(ml_get_buf(curbuf, curwin->w_cursor.lnum, TRUE)
						  + curwin->w_cursor.col) = c;
}

#if 0 /* not used */
/*
 * Put *pos at end of current buffer
 */
    void
goto_endofbuf(pos)
    pos_T    *pos;
{
    char_u  *p;

    pos->lnum = curbuf->b_ml.ml_line_count;
    pos->col = 0;
    p = ml_get(pos->lnum);
    while (*p++)
	++pos->col;
}
#endif

/*
 * When extra == 0: Return TRUE if the cursor is before or on the first
 *		    non-blank in the line.
 * When extra == 1: Return TRUE if the cursor is before the first non-blank in
 *		    the line.
 */
    int
inindent(extra)
    int	    extra;
{
    char_u	*ptr;
    colnr_T	col;

    for (col = 0, ptr = ml_get_curline(); vim_iswhite(*ptr); ++col)
	++ptr;
    if (col >= curwin->w_cursor.col + extra)
	return TRUE;
    else
	return FALSE;
}

/*
 * Skip to next part of an option argument: Skip space and comma.
 */
    char_u *
skip_to_option_part(p)
    char_u  *p;
{
    if (*p == ',')
	++p;
    while (*p == ' ')
	++p;
    return p;
}

/*
 * changed() is called when something in the current buffer is changed.
 *
 * Most often called through changed_bytes() and changed_lines(), which also
 * mark the area of the display to be redrawn.
 */
    void
changed()
{
    int	    save_msg_scroll = msg_scroll;

    if (!curbuf->b_changed)
    {
	change_warning(0);
	/* Create a swap file if that is wanted.
	 * Don't do this for "nofile" and "nowrite" buffer types. */
	if (curbuf->b_may_swap
#ifdef FEAT_QUICKFIX
		&& !bt_dontwrite(curbuf)
#endif
		)
	{
	    ml_open_file(curbuf);

	    /* The ml_open_file() can cause an ATTENTION message.
	     * Wait two seconds, to make sure the user reads this unexpected
	     * message.  Since we could be anywhere, call wait_return() now,
	     * and don't let the emsg() set msg_scroll. */
	    if (need_wait_return && emsg_silent == 0)
	    {
		out_flush();
		ui_delay(2000L, TRUE);
		wait_return(TRUE);
		msg_scroll = save_msg_scroll;
	    }
	}
	curbuf->b_changed = TRUE;
	ml_setdirty(curbuf, TRUE);
#ifdef FEAT_WINDOWS
	check_status(curbuf);
#endif
#ifdef FEAT_TITLE
	need_maketitle = TRUE;	    /* set window title later */
#endif
    }
    ++curbuf->b_changedtick;
    ++global_changedtick;
}

static void changedOneline __ARGS((linenr_T lnum));
static void changed_common __ARGS((linenr_T lnum, colnr_T col, linenr_T lnume, long xtra));

/*
 * Changed bytes within a single line for the current buffer.
 * - marks the windows on this buffer to be redisplayed
 * - marks the buffer changed by calling changed()
 * - invalidates cached values
 */
    void
changed_bytes(lnum, col)
    linenr_T	lnum;
    colnr_T	col;
{
    changedOneline(lnum);
    changed_common(lnum, col, lnum + 1, 0L);
}

    static void
changedOneline(lnum)
    linenr_T	lnum;
{
    if (curbuf->b_mod_set)
    {
	/* find the maximum area that must be redisplayed */
	if (lnum < curbuf->b_mod_top)
	    curbuf->b_mod_top = lnum;
	else if (lnum >= curbuf->b_mod_bot)
	    curbuf->b_mod_bot = lnum + 1;
    }
    else
    {
	/* set the area that must be redisplayed to one line */
	curbuf->b_mod_set = TRUE;
	curbuf->b_mod_top = lnum;
	curbuf->b_mod_bot = lnum + 1;
	curbuf->b_mod_xlines = 0;
    }
}

/*
 * Appended "count" lines below line "lnum" in the current buffer.
 * Must be called AFTER the change and after mark_adjust().
 * Takes care of marking the buffer to be redrawn and sets the changed flag.
 */
    void
appended_lines(lnum, count)
    linenr_T	lnum;
    long	count;
{
    changed_lines(lnum + 1, 0, lnum + 1, count);
}

/*
 * Like appended_lines(), but adjust marks first.
 */
    void
appended_lines_mark(lnum, count)
    linenr_T	lnum;
    long	count;
{
    mark_adjust(lnum + 1, (linenr_T)MAXLNUM, count, 0L);
    changed_lines(lnum + 1, 0, lnum + 1, count);
}

/*
 * Deleted "count" lines at line "lnum" in the current buffer.
 * Must be called AFTER the change and after mark_adjust().
 * Takes care of marking the buffer to be redrawn and sets the changed flag.
 */
    void
deleted_lines(lnum, count)
    linenr_T	lnum;
    long	count;
{
    changed_lines(lnum, 0, lnum + count, -count);
}

/*
 * Like deleted_lines(), but adjust marks first.
 */
    void
deleted_lines_mark(lnum, count)
    linenr_T	lnum;
    long	count;
{
    mark_adjust(lnum, (linenr_T)(lnum + count - 1), (long)MAXLNUM, -count);
    changed_lines(lnum, 0, lnum + count, -count);
}

/*
 * Changed lines for the current buffer.
 * Must be called AFTER the change and after mark_adjust().
 * - mark the buffer changed by calling changed()
 * - mark the windows on this buffer to be redisplayed
 * - invalidate cached values
 * "lnum" is the first line that needs displaying, "lnume" the first line
 * below the changed lines (BEFORE the change).
 * When only inserting lines, "lnum" and "lnume" are equal.
 * Takes care of calling changed() and updating b_mod_*.
 */
    void
changed_lines(lnum, col, lnume, xtra)
    linenr_T	lnum;	    /* first line with change */
    colnr_T	col;	    /* column in first line with change */
    linenr_T	lnume;	    /* line below last changed line */
    long	xtra;	    /* number of extra lines (negative when deleting) */
{
    if (curbuf->b_mod_set)
    {
	/* find the maximum area that must be redisplayed */
	if (lnum < curbuf->b_mod_top)
	    curbuf->b_mod_top = lnum;
	if (lnum < curbuf->b_mod_bot)
	{
	    /* adjust old bot position for xtra lines */
	    curbuf->b_mod_bot += xtra;
	    if (curbuf->b_mod_bot < lnum)
		curbuf->b_mod_bot = lnum;
	}
	if (lnume + xtra > curbuf->b_mod_bot)
	    curbuf->b_mod_bot = lnume + xtra;
	curbuf->b_mod_xlines += xtra;
    }
    else
    {
	/* set the area that must be redisplayed */
	curbuf->b_mod_set = TRUE;
	curbuf->b_mod_top = lnum;
	curbuf->b_mod_bot = lnume + xtra;
	curbuf->b_mod_xlines = xtra;
    }

    changed_common(lnum, col, lnume, xtra);
}

    static void
changed_common(lnum, col, lnume, xtra)
    linenr_T	lnum;
    colnr_T	col;
    linenr_T	lnume;
    long	xtra;
{
    win_T	*wp;
    int		i;

    /* mark the buffer as modified */
    changed();

    /* set the '. mark */
    curbuf->b_last_change.lnum = lnum;
    curbuf->b_last_change.col = col;

    FOR_ALL_WINDOWS(wp)
    {
	if (wp->w_buffer == curbuf)
	{
	    /* Mark this window to be redrawn later. */
	    if (wp->w_redr_type < VALID)
		wp->w_redr_type = VALID;

	    /* Check if a change in the buffer has invalidated the cached
	     * values for the cursor. */
#ifdef FEAT_FOLDING
	    /*
	     * Update the folds for this window.  Can't postpone this, because
	     * a following operator might work on the whole fold: ">>dd".
	     */
	    foldUpdate(wp, lnum, lnume + xtra - 1);

	    /* The change may cause lines above or below the change to become
	     * included in a fold.  Set lnum/lnume to the first/last line that
	     * might be displayed differently. */
	    hasFoldingWin(wp, lnum, &lnum, NULL, FALSE, NULL);
	    hasFoldingWin(wp, lnume, NULL, &lnume, FALSE, NULL);

	    /* If the changed line is in a range of previously folded lines,
	     * compare with the first line in that range. */
	    if (wp->w_cursor.lnum <= lnum)
	    {
		i = find_wl_entry(wp, lnum);
		if (i >= 0 && wp->w_cursor.lnum > wp->w_lines[i].wl_lnum)
		    changed_line_abv_curs_win(wp);
	    }
#endif

	    if (wp->w_cursor.lnum > lnum)
		changed_line_abv_curs_win(wp);
	    else if (wp->w_cursor.lnum == lnum && wp->w_cursor.col >= col)
		changed_cline_bef_curs_win(wp);
	    if (wp->w_botline >= lnum)
	    {
		/* Assume that botline doesn't change (inserted lines make
		 * other lines scroll down below botline). */
		approximate_botline_win(wp);
	    }

	    /* Check if any w_lines[] entries have become invalid.
	     * For entries below the change: Correct the lnums for
	     * inserted/deleted lines.  Makes it possible to stop displaying
	     * after the change. */
	    for (i = 0; i < wp->w_lines_valid; ++i)
		if (wp->w_lines[i].wl_valid)
		{
		    if (wp->w_lines[i].wl_lnum >= lnum)
		    {
			if (wp->w_lines[i].wl_lnum < lnume)
			{
			    /* line included in change */
			    wp->w_lines[i].wl_valid = FALSE;
			}
			else if (xtra != 0)
			{
			    /* line below change */
			    wp->w_lines[i].wl_lnum += xtra;
#ifdef FEAT_FOLDING
			    wp->w_lines[i].wl_lastlnum += xtra;
#endif
			}
		    }
#ifdef FEAT_FOLDING
		    else if (wp->w_lines[i].wl_lastlnum >= lnum)
		    {
			/* change somewhere inside this range of folded lines,
			 * may need to be redrawn */
			wp->w_lines[i].wl_valid = FALSE;
		    }
#endif
		}
	}
    }

    /* Call update_screen() later, which checks out what needs to be redrawn,
     * since it notices b_mod_set and then uses b_mod_*. */
    if (must_redraw < VALID)
	must_redraw = VALID;
}

/*
 * unchanged() is called when the changed flag must be reset for buffer 'buf'
 */
    void
unchanged(buf, ff)
    buf_T	*buf;
    int		ff;	/* also reset 'fileformat' */
{
    if (buf->b_changed || (ff && file_ff_differs(buf)))
    {
	buf->b_changed = 0;
	ml_setdirty(buf, FALSE);
	if (ff)
	    save_file_ff(buf);
#ifdef FEAT_WINDOWS
	check_status(buf);
#endif
#ifdef FEAT_TITLE
	need_maketitle = TRUE;	    /* set window title later */
#endif
    }
    ++buf->b_changedtick;
    ++global_changedtick;
}

#if defined(FEAT_WINDOWS) || defined(PROTO)
/*
 * check_status: called when the status bars for the buffer 'buf'
 *		 need to be updated
 */
    void
check_status(buf)
    buf_T	*buf;
{
    win_T	*wp;

    for (wp = firstwin; wp != NULL; wp = wp->w_next)
	if (wp->w_buffer == buf && wp->w_status_height)
	{
	    wp->w_redr_status = TRUE;
	    if (must_redraw < VALID)
		must_redraw = VALID;
	}
}
#endif

/*
 * If the file is readonly, give a warning message with the first change.
 * Don't do this for autocommands.
 * Don't use emsg(), because it flushes the macro buffer.
 * If we have undone all changes b_changed will be FALSE, but b_did_warn
 * will be TRUE.
 */
    void
change_warning(col)
    int	    col;		/* column for message; non-zero when in insert
				   mode and 'showmode' is on */
{
    if (curbuf->b_did_warn == FALSE
	    && curbufIsChanged() == 0
#ifdef FEAT_AUTOCMD
	    && !autocmd_busy
#endif
	    && curbuf->b_p_ro)
    {
#ifdef FEAT_AUTOCMD
	apply_autocmds(EVENT_FILECHANGEDRO, NULL, NULL, FALSE, curbuf);
	if (!curbuf->b_p_ro)
	    return;
#endif
	/*
	 * Do what msg() does, but with a column offset if the warning should
	 * be after the mode message.
	 */
	msg_start();
	if (msg_row == Rows - 1)
	    msg_col = col;
	MSG_PUTS_ATTR(_("W10: Warning: Changing a readonly file"),
						   hl_attr(HLF_W) | MSG_HIST);
	msg_clr_eos();
	(void)msg_end();
	if (msg_silent == 0)
	{
	    out_flush();
	    ui_delay(1000L, TRUE); /* give the user time to think about it */
	}
	curbuf->b_did_warn = TRUE;
	redraw_cmdline = FALSE;	/* don't redraw and erase the message */
	if (msg_row < Rows - 1)
	    showmode();
    }
}

/*
 * Ask for a reply from the user, a 'y' or a 'n'.
 * No other characters are accepted, the message is repeated until a valid
 * reply is entered or CTRL-C is hit.
 * If direct is TRUE, don't use vgetc() but ui_inchar(), don't get characters
 * from any buffers but directly from the user.
 *
 * return the 'y' or 'n'
 */
    int
ask_yesno(str, direct)
    char_u  *str;
    int	    direct;
{
    int	    r = ' ';
    int	    save_State = State;

    if (exiting)		/* put terminal in raw mode for this question */
	settmode(TMODE_RAW);
    ++no_wait_return;
#ifdef USE_ON_FLY_SCROLL
    dont_scroll = TRUE;		/* disallow scrolling here */
#endif
    State = CONFIRM;		/* mouse behaves like with :confirm */
#ifdef FEAT_MOUSE
    setmouse();			/* disables mouse for xterm */
#endif
    ++no_mapping;
    ++allow_keys;		/* no mapping here, but recognize keys */

    while (r != 'y' && r != 'n')
    {
	/* same highlighting as for wait_return */
	smsg_attr(hl_attr(HLF_R), (char_u *)"%s (y/n)?", str);
	if (direct)
	    r = get_keystroke();
	else
	    r = safe_vgetc();
	if (r == Ctrl_C || r == ESC)
	    r = 'n';
	msg_putchar(r);	    /* show what you typed */
	out_flush();
    }
    --no_wait_return;
    State = save_State;
#ifdef FEAT_MOUSE
    setmouse();
#endif
    --no_mapping;
    --allow_keys;

    return r;
}

/*
 * Get a key stroke directly from the user.
 * Ignores mouse clicks and scrollbar events.
 * Doesn't use vgetc(), because it syncs undo and eats mapped characters.
 * Disadvantage: typeahead is ignored.
 * Translates the interrupt character for unix to ESC.
 */
    int
get_keystroke()
{
#define CBUFLEN 151
    char_u	buf[CBUFLEN];
    int		len = 0;
    int		n;

    for (;;)
    {
	cursor_on();
	out_flush();

	/* First time: blocking wait.  Second time: wait up to 100ms for a
	 * terminal code to complete.  Leave some room for check_termcode() to
	 * insert a key code into (max 5 chars plus NUL).  And
	 * fix_input_buffer() can triple the number of bytes. */
	n = ui_inchar(buf + len, (CBUFLEN - 6 - len) / 3,
						       len == 0 ? -1L : 100L);
	if (n > 0)
	{
	    /* Replace zero and CSI by a special key code. */
	    n = fix_input_buffer(buf + len, n, FALSE);
	    len += n;
	}

	/* incomplete termcode: get more characters */
	if ((n = check_termcode(1, buf, len)) < 0)
	    continue;
	/* found a termcode: adjust length */
	if (n > 0)
	    len = n;
	if (len == 0)	    /* nothing typed yet */
	    continue;

	/* Handle modifier and/or special key code. */
	n = buf[0];
	if (n == K_SPECIAL)
	{
	    n = TO_SPECIAL(buf[1], buf[2]);
	    if (buf[1] == KS_MODIFIER
		    || n == K_IGNORE
		    || n == K_SILENT
#ifdef FEAT_MOUSE
		    || n == K_LEFTMOUSE
		    || n == K_LEFTMOUSE_NM
		    || n == K_LEFTDRAG
		    || n ==  K_LEFTRELEASE
		    || n ==  K_LEFTRELEASE_NM
		    || n ==  K_MIDDLEMOUSE
		    || n ==  K_MIDDLEDRAG
		    || n ==  K_MIDDLERELEASE
		    || n ==  K_RIGHTMOUSE
		    || n ==  K_RIGHTDRAG
		    || n ==  K_RIGHTRELEASE
		    || n ==  K_MOUSEDOWN
		    || n ==  K_MOUSEUP
# ifdef FEAT_GUI
		    || n == K_VER_SCROLLBAR
		    || n == K_HOR_SCROLLBAR
# endif
#endif
	       )
	    {
		if (buf[1] == KS_MODIFIER)
		    mod_mask = buf[2];
		len -= 3;
		if (len > 0)
		    mch_memmove(buf, buf + 3, (size_t)len);
		continue;
	    }
	}
#ifdef UNIX
	if (n == intr_char)
	    n = ESC;
#endif
	break;
    }
    return n;
}

/*
 * get a number from the user
 */
    int
get_number(colon)
    int	colon;			/* allow colon to abort */
{
    int	n = 0;
    int	c;

#ifdef USE_ON_FLY_SCROLL
    dont_scroll = TRUE;		/* disallow scrolling here */
#endif
    ++no_mapping;
    ++allow_keys;		/* no mapping here, but recognize keys */
    for (;;)
    {
	windgoto(msg_row, msg_col);
	c = safe_vgetc();
	if (vim_isdigit(c))
	{
	    n = n * 10 + c - '0';
	    msg_putchar(c);
	}
	else if (c == K_DEL || c == K_KDEL || c == K_BS || c == Ctrl_H)
	{
	    n /= 10;
	    MSG_PUTS("\b \b");
	}
	else if (n == 0 && c == ':' && colon)
	{
	    stuffcharReadbuff(':');
	    if (!exmode_active)
		cmdline_row = msg_row;
	    skip_redraw = TRUE;	    /* skip redraw once */
	    do_redraw = FALSE;
	    break;
	}
	else if (c == CR || c == NL || c == Ctrl_C || c == ESC)
	    break;
    }
    --no_mapping;
    --allow_keys;
    return n;
}

    void
msgmore(n)
    long n;
{
    long pn;

    if (global_busy	    /* no messages now, wait until global is finished */
	    || keep_msg != NULL /* there is a message already, skip this one */
	    || !messaging())  /* 'lazyredraw' set, don't do messages now */
	return;

    if (n > 0)
	pn = n;
    else
	pn = -n;

    if (pn > p_report)
    {
	if (pn == 1)
	{
	    if (n > 0)
		STRCPY(msg_buf, _("1 more line"));
	    else
		STRCPY(msg_buf, _("1 line less"));
	}
	else
	{
	    if (n > 0)
		sprintf((char *)msg_buf, _("%ld more lines"), pn);
	    else
		sprintf((char *)msg_buf, _("%ld fewer lines"), pn);
	}
	if (got_int)
	    STRCAT(msg_buf, _(" (Interrupted)"));
	if (msg(msg_buf))
	{
	    set_keep_msg(msg_buf);
	    keep_msg_attr = 0;
	}
    }
}

/*
 * flush map and typeahead buffers and give a warning for an error
 */
    void
beep_flush()
{
    if (emsg_silent == 0)
    {
	flush_buffers(FALSE);
	vim_beep();
    }
}

/*
 * give a warning for an error
 */
    void
vim_beep()
{
    if (emsg_silent == 0)
    {
	if (p_vb)
	{
	    out_str(T_VB);
	}
	else
	{
#ifdef MSDOS
	    /*
	     * The number of beeps outputted is reduced to avoid having to wait
	     * for all the beeps to finish. This is only a problem on systems
	     * where the beeps don't overlap.
	     */
	    if (beep_count == 0 || beep_count == 10)
	    {
		out_char('\007');
		beep_count = 1;
	    }
	    else
		++beep_count;
#else
	    out_char('\007');
#endif
	}
    }
}

/*
 * To get the "real" home directory:
 * - get value of $HOME
 * For Unix:
 *  - go to that directory
 *  - do mch_dirname() to get the real name of that directory.
 *  This also works with mounts and links.
 *  Don't do this for MS-DOS, it will change the "current dir" for a drive.
 */
static char_u	*homedir = NULL;

    void
init_homedir()
{
    char_u  *var;

#ifdef VMS
    var = mch_getenv((char_u *)"SYS$LOGIN");
#else
    var = mch_getenv((char_u *)"HOME");
#endif

    if (var != NULL && *var == NUL)	/* empty is same as not set */
	var = NULL;

#ifdef WIN3264
    /*
     * Typically, $HOME is not defined on Windows, unless the user has
     * specifically defined it for Vim's sake.  However, on Windows NT
     * platforms, $HOMEDRIVE and $HOMEPATH are automatically defined for
     * each user.  Try constructing $HOME from these.
     */
    if (var == NULL)
    {
        char_u *homedrive, *homepath;

        homedrive = mch_getenv((char_u *)"HOMEDRIVE");
        homepath = mch_getenv((char_u *)"HOMEPATH");
        if (homedrive != NULL && homepath != NULL)
        {
            sprintf((char *)NameBuff, "%s%s", homedrive, homepath);
            if (NameBuff[0] != NUL)
                var = NameBuff;
        }
    }
#endif

#if defined(OS2) || defined(MSDOS) || defined(MSWIN)
    /*
     * Default home dir is C:/
     * Best assumption we can make in such a situation.
     */
    if (var == NULL)
	var = "C:/";
#endif
    if (var != NULL)
    {
#ifdef UNIX
	/*
	 * Change to the directory and get the actual path.  This resolves
	 * links.
	 */
	if (mch_dirname(NameBuff, MAXPATHL) == OK)
	{
	    if (!mch_chdir((char *)var) && mch_dirname(IObuff, IOSIZE) == OK)
		var = IObuff;
	    mch_chdir((char *)NameBuff);
	}
#endif
	homedir = vim_strsave(var);
    }
}

/*
 * Expand environment variable with path name.
 * "~/" is also expanded, using $HOME.	For Unix "~user/" is expanded.
 * Skips over "\ ", "\~" and "\$".
 * If anything fails no expansion is done and dst equals src.
 */
    void
expand_env(src, dst, dstlen)
    char_u	*src;		/* input string e.g. "$HOME/vim.hlp" */
    char_u	*dst;		/* where to put the result */
    int		dstlen;		/* maximum length of the result */
{
    char_u	*tail;
    int		c;
    char_u	*var;
    int		copy_char;
    int		mustfree;	/* var was allocated, need to free it later */
    int		at_start = TRUE; /* at start of a name */

    src = skipwhite(src);
    --dstlen;		    /* leave one char space for "\," */
    while (*src && dstlen > 0)
    {
	copy_char = TRUE;
	if (*src == '$'
#if defined(MSDOS) || defined(MSWIN) || defined(OS2)
		|| *src == '%'
#endif
		|| (*src == '~' && at_start))
	{
	    mustfree = FALSE;

	    /*
	     * The variable name is copied into dst temporarily, because it may
	     * be a string in read-only memory and a NUL needs to be appended.
	     */
	    if (*src != '~')				/* environment var */
	    {
		tail = src + 1;
		var = dst;
		c = dstlen - 1;

#ifdef UNIX
		/* Unix has ${var-name} type environment vars */
		if (*tail == '{' && !vim_isIDc('{'))
		{
		    tail++;	/* ignore '{' */
		    while (c-- > 0 && *tail && *tail != '}')
			*var++ = *tail++;
		}
		else
#endif
		{
		    while (c-- > 0 && *tail != NUL && ((vim_isIDc(*tail))
#if defined(MSDOS) || defined(MSWIN) || defined(OS2)
			    || (*src == '%' && *tail != '%')
#endif
			    ))
		    {
#ifdef OS2		/* env vars only in uppercase */
			*var++ = TO_UPPER(*tail);
			tail++;	    /* toupper() may be a macro! */
#else
			*var++ = *tail++;
#endif
		    }
		}

#if defined(MSDOS) || defined(MSWIN) || defined(OS2) || defined(UNIX)
# ifdef UNIX
		if (src[1] == '{' && *tail != '}')
# else
		if (*src == '%' && *tail != '%')
# endif
		    var = NULL;
		else
		{
# ifdef UNIX
		    if (src[1] == '{')
# else
		    if (*src == '%')
#endif
			++tail;
#endif
		    *var = NUL;
		    var = vim_getenv(dst, &mustfree);
#if defined(MSDOS) || defined(MSWIN) || defined(OS2) || defined(UNIX)
		}
#endif
	    }
							/* home directory */
	    else if (  src[1] == NUL
		    || vim_ispathsep(src[1])
		    || vim_strchr((char_u *)" ,\t\n", src[1]) != NULL)
	    {
		var = homedir;
		tail = src + 1;
	    }
	    else					/* user directory */
	    {
#if defined(UNIX) || (defined(VMS) && defined(USER_HOME))
		/*
		 * Copy ~user to dst[], so we can put a NUL after it.
		 */
		tail = src;
		var = dst;
		c = dstlen - 1;
		while (	   c-- > 0
			&& *tail
			&& vim_isfilec(*tail)
			&& !vim_ispathsep(*tail))
		    *var++ = *tail++;
		*var = NUL;
# ifdef UNIX
		/*
		 * If the system supports getpwnam(), use it.
		 * Otherwise, or if getpwnam() fails, the shell is used to
		 * expand ~user.  This is slower and may fail if the shell
		 * does not support ~user (old versions of /bin/sh).
		 */
#  if defined(HAVE_GETPWNAM) && defined(HAVE_PWD_H)
		{
		    struct passwd *pw;

		    pw = getpwnam((char *)dst + 1);
		    if (pw != NULL)
			var = (char_u *)pw->pw_dir;
		    else
			var = NULL;
		}
		if (var == NULL)
#  endif
		{
		    expand_T	xpc;

		    xpc.xp_context = EXPAND_FILES;
		    var = ExpandOne(&xpc, dst, NULL,
				WILD_ADD_SLASH|WILD_SILENT, WILD_EXPAND_FREE);
		    mustfree = TRUE;
		}

# else	/* !UNIX, thus VMS */
		/*
		 * USER_HOME is a comma-separated list of
		 * directories to search for the user account in.
		 */
		{
		    char_u	test[MAXPATHL], paths[MAXPATHL];
		    char_u	*path, *next_path, *ptr;
		    struct stat	st;

		    STRCPY(paths, USER_HOME);
		    next_path = paths;
		    while (*next_path)
		    {
			for (path = next_path; *next_path && *next_path != ',';
				next_path++);
			if (*next_path)
			    *next_path++ = NUL;
			STRCPY(test, path);
			STRCAT(test, "/");
			STRCAT(test, dst + 1);
			if (mch_stat(test, &st) == 0)
			{
			    var = alloc(STRLEN(test) + 1);
			    STRCPY(var, test);
			    mustfree = TRUE;
			    break;
			}
		    }
		}
# endif /* UNIX */
#else
		/* cannot expand user's home directory, so don't try */
		var = NULL;
		tail = (char_u *)"";	/* for gcc */
#endif /* UNIX || VMS */
	    }

	    if (var != NULL && *var != NUL
		    && (STRLEN(var) + STRLEN(tail) + 1 < (unsigned)dstlen))
	    {
		STRCPY(dst, var);
		dstlen -= (int)STRLEN(var);
		dst += STRLEN(var);
		/* if var[] ends in a path separator and tail[] starts
		 * with it, skip a character */
		if (*var != NUL && vim_ispathsep(dst[-1])
#if defined(BACKSLASH_IN_FILENAME) || defined(AMIGA)
			&& dst[-1] != ':'
#endif
			&& vim_ispathsep(*tail))
		    ++tail;
		src = tail;
		copy_char = FALSE;
	    }
	    if (mustfree)
		vim_free(var);
	}

	if (copy_char)	    /* copy at least one char */
	{
	    /*
	     * Recogize the start of a new name, for '~'.
	     */
	    at_start = FALSE;
	    if (src[0] == '\\')
	    {
		*dst++ = *src++;
		--dstlen;
	    }
	    else if (src[0] == ' ' || src[0] == ',')
		at_start = TRUE;
	    *dst++ = *src++;
	    --dstlen;
	}
    }
    *dst = NUL;
}

/*
 * Vim's version of getenv().
 * Special handling of $HOME, $VIM and $VIMRUNTIME.
 */
    char_u *
vim_getenv(name, mustfree)
    char_u	*name;
    int		*mustfree;	/* set to TRUE when returned is allocated */
{
    char_u	*p;
    char_u	*pend;
    int		vimruntime;

#if defined(OS2) || defined(MSDOS) || defined(MSWIN)
    /* use "C:/" when $HOME is not set */
    if (STRCMP(name, "HOME") == 0)
	return homedir;
#endif

    p = mch_getenv(name);
    if (p != NULL && *p == NUL)	    /* empty is the same as not set */
	p = NULL;

    if (p != NULL)
	return p;

    vimruntime = (STRCMP(name, "VIMRUNTIME") == 0);
    if (!vimruntime && STRCMP(name, "VIM") != 0)
	return NULL;

    /*
     * When expanding $VIMRUNTIME fails, try using $VIM/vim<version> or $VIM.
     * Don't do this when default_vimruntime_dir is non-empty.
     */
    if (vimruntime
#ifdef HAVE_PATHDEF
	    && *default_vimruntime_dir == NUL
#endif
       )
    {
	p = mch_getenv((char_u *)"VIM");
	if (p != NULL && *p == NUL)	    /* empty is the same as not set */
	    p = NULL;
	if (p != NULL)
	{
	    p = vim_version_dir(p);
	    if (p != NULL)
		*mustfree = TRUE;
	    else
		p = mch_getenv((char_u *)"VIM");
	}
    }

    /*
     * When expanding $VIM or $VIMRUNTIME fails, try using:
     * - the directory name from 'helpfile' (unless it contains '$')
     * - the executable name from argv[0]
     */
    if (p == NULL)
    {
	if (p_hf != NULL && vim_strchr(p_hf, '$') == NULL)
	    p = p_hf;
#ifdef USE_EXE_NAME
	/*
	 * Use the name of the executable, obtained from argv[0].
	 */
	else
	    p = exe_name;
#endif
	if (p != NULL)
	{
	    /* remove the file name */
	    pend = gettail(p);

	    /* remove "doc/" from 'helpfile', if present */
	    if (p == p_hf)
		pend = remove_tail(p, pend, (char_u *)"doc");

#ifdef USE_EXE_NAME
# ifdef MACOS_X
            /* remove "build/..." from exe_name, if present */
            if (p == exe_name)
	    {
		pend = remove_tail(p, pend, (char_u *)"Contents/MacOS");
		pend = remove_tail_with_ext(p, pend, (char_u *)".app");
		pend = remove_tail(p, pend, (char_u *)"build");
	    }
# endif
	    /* remove "src/" from exe_name, if present */
	    if (p == exe_name)
		pend = remove_tail(p, pend, (char_u *)"src");
#endif

	    /* for $VIM, remove "runtime/" or "vim54/", if present */
	    if (!vimruntime)
	    {
		pend = remove_tail(p, pend, (char_u *)RUNTIME_DIRNAME);
		pend = remove_tail(p, pend, (char_u *)VIM_VERSION_NODOT);
	    }

	    /* remove trailing path separator */
#ifndef MACOS_CLASSIC
	    /* With MacOS path (with  colons) the final colon is required */
            /* to avoid confusin between absoulute and relative path */
	    if (pend > p && vim_ispathsep(*(pend - 1)))
		--pend;
#endif

	    /* check that the result is a directory name */
	    p = vim_strnsave(p, (int)(pend - p));

	    if (p != NULL && !mch_isdir(p))
	    {
		vim_free(p);
		p = NULL;
	    }
	    else
	    {
#ifdef USE_EXE_NAME
		/* may add "/vim54" or "/runtime" if it exists */
		if (vimruntime && (pend = vim_version_dir(p)) != NULL)
		{
		    vim_free(p);
		    p = pend;
		}
#endif
		*mustfree = TRUE;
	    }
	}
    }

#ifdef HAVE_PATHDEF
    /* When there is a pathdef.c file we can use default_vim_dir and
     * default_vimruntime_dir */
    if (p == NULL)
    {
	/* Only use default_vimruntime_dir when it is not empty */
	if (vimruntime && *default_vimruntime_dir != NUL)
	{
	    p = default_vimruntime_dir;
	    *mustfree = FALSE;
	}
	else if (*default_vim_dir != NUL)
	{
	    if (vimruntime && (p = vim_version_dir(default_vim_dir)) != NULL)
		*mustfree = TRUE;
	    else
	    {
		p = default_vim_dir;
		*mustfree = FALSE;
	    }
	}
    }
#endif

    /*
     * Set the environment variable, so that the new value can be found fast
     * next time, and others can also use it (e.g. Perl).
     */
    if (p != NULL)
    {
	if (vimruntime)
	{
	    vim_setenv((char_u *)"VIMRUNTIME", p);
	    didset_vimruntime = TRUE;
#ifdef FEAT_GETTEXT
	    {
		char_u	*buf = alloc((unsigned int)STRLEN(p) + 6);

		if (buf != NULL)
		{
		    STRCPY(buf, p);
		    STRCAT(buf, "/lang");
		    bindtextdomain(VIMPACKAGE, (char *)buf);
		    vim_free(buf);
		}
	    }
#endif
	}
	else
	{
	    vim_setenv((char_u *)"VIM", p);
	    didset_vim = TRUE;
	}
    }
    return p;
}

/*
 * Check if the directory "vimdir/<version>" or "vimdir/runtime" exists.
 * Return NULL if not, return its name in allocated memory otherwise.
 */
    static char_u *
vim_version_dir(vimdir)
    char_u	*vimdir;
{
    char_u	*p;

    if (vimdir == NULL || *vimdir == NUL)
	return NULL;
    p = concat_fnames(vimdir, (char_u *)VIM_VERSION_NODOT, TRUE);
    if (p != NULL && mch_isdir(p))
	return p;
    vim_free(p);
    p = concat_fnames(vimdir, (char_u *)RUNTIME_DIRNAME, TRUE);
    if (p != NULL && mch_isdir(p))
	return p;
    vim_free(p);
    return NULL;
}

/*
 * If the string between "p" and "pend" ends in "name/", return "pend" minus
 * the length of "name/".  Otherwise return "pend".
 */
    static char_u *
remove_tail(p, pend, name)
    char_u	*p;
    char_u	*pend;
    char_u	*name;
{
    int		len = (int)STRLEN(name) + 1;
    char_u	*newend = pend - len;

    if (newend >= p
	    && fnamencmp(newend, name, len - 1) == 0
	    && (newend == p || vim_ispathsep(*(newend - 1))))
	return newend;
    return pend;
}

#if defined(USE_EXE_NAME) && defined(MACOS_X)
/*
 * If the string between "p" and "pend" ends in "???.ext/", return "pend" minus
 * the length of "???.ext/".  Otherwise return "pend".
 */
    static char_u *
remove_tail_with_ext(p, pend, ext)
    char_u	*p;
    char_u	*pend;
    char_u	*ext;
{
    int		len = (int)STRLEN(ext) + 1;
    char_u	*newend = pend - len;

    if (newend >= p
	    && fnamencmp(newend, ext, len - 1) == 0)
	for (;newend != p && !vim_ispathsep(*(newend -1)); newend--);

    if (newend == p || vim_ispathsep(*(newend - 1)))
	return newend;
    return pend;
}
#endif

/*
 * Call expand_env() and store the result in an allocated string.
 * This is not very memory efficient, this expects the result to be freed
 * again soon.
 */
    char_u *
expand_env_save(src)
    char_u	*src;
{
    char_u	*p;

    p = alloc(MAXPATHL);
    if (p != NULL)
	expand_env(src, p, MAXPATHL);
    return p;
}

/*
 * Our portable version of setenv.
 */
    void
vim_setenv(name, val)
    char_u	*name;
    char_u	*val;
{
#ifdef HAVE_SETENV
    mch_setenv((char *)name, (char *)val, 1);
#else
    char_u	*envbuf;

    /*
     * Putenv does not copy the string, it has to remain
     * valid.  The allocated memory will never be freed.
     */
    envbuf = alloc((unsigned)(STRLEN(name) + STRLEN(val) + 2));
    if (envbuf != NULL)
    {
	sprintf((char *)envbuf, "%s=%s", name, val);
	putenv((char *)envbuf);
    }
#endif
}

#if defined(FEAT_CMDL_COMPL) || defined(PROTO)
/*
 * Function given to ExpandGeneric() to obtain an environment variable name.
 */
/*ARGSUSED*/
    char_u *
get_env_name(xp, idx)
    expand_T	*xp;
    int		idx;
{
# if defined(AMIGA) || defined(__MRC__) || defined(__SC__)
    /*
     * No environ[] on the Amiga and on the Mac (using MPW).
     */
    return NULL;
# else
# ifndef __WIN32__
    /* Borland C++ 5.2 has this in a header file. */
    extern char		**environ;
# endif
    static char_u	name[100];
    char_u		*str;
    int			n;

    str = (char_u *)environ[idx];
    if (str == NULL)
	return NULL;

    for (n = 0; n < 99; ++n)
    {
	if (str[n] == '=' || str[n] == NUL)
	    break;
	name[n] = str[n];
    }
    name[n] = NUL;
    return name;
# endif
}
#endif

/*
 * Replace home directory by "~" in each space or comma separated file name in
 * 'src'.
 * If anything fails (except when out of space) dst equals src.
 */
    void
home_replace(buf, src, dst, dstlen, one)
    buf_T	*buf;	/* when not NULL, check for help files */
    char_u	*src;	/* input file name */
    char_u	*dst;	/* where to put the result */
    int		dstlen;	/* maximum length of the result */
    int		one;	/* if TRUE, only replace one file name, include
			   spaces and commas in the file name. */
{
    size_t	dirlen = 0, envlen = 0;
    size_t	len;
    char_u	*homedir_env;
    char_u	*p;

    if (src == NULL)
    {
	*dst = NUL;
	return;
    }

    /*
     * If the file is a help file, remove the path completely.
     */
    if (buf != NULL && buf->b_help)
    {
	STRCPY(dst, gettail(src));
	return;
    }

    /*
     * We check both the value of the $HOME environment variable and the
     * "real" home directory.
     */
    if (homedir != NULL)
	dirlen = STRLEN(homedir);

#ifdef VMS
    homedir_env = mch_getenv((char_u *)"SYS$LOGIN");
#else
    homedir_env = mch_getenv((char_u *)"HOME");
#endif

    if (homedir_env != NULL && *homedir_env == NUL)
	homedir_env = NULL;
    if (homedir_env != NULL)
	envlen = STRLEN(homedir_env);

    if (!one)
	src = skipwhite(src);
    while (*src && dstlen > 0)
    {
	/*
	 * Here we are at the beginning of a file name.
	 * First, check to see if the beginning of the file name matches
	 * $HOME or the "real" home directory. Check that there is a '/'
	 * after the match (so that if e.g. the file is "/home/pieter/bla",
	 * and the home directory is "/home/piet", the file does not end up
	 * as "~er/bla" (which would seem to indicate the file "bla" in user
	 * er's home directory)).
	 */
	p = homedir;
	len = dirlen;
	for (;;)
	{
	    if (   len
		&& fnamencmp(src, p, len) == 0
		&& (vim_ispathsep(src[len])
		    || (!one && (src[len] == ',' || src[len] == ' '))
		    || src[len] == NUL))
	    {
		src += len;
		if (--dstlen > 0)
		    *dst++ = '~';

		/*
		 * If it's just the home directory, add  "/".
		 */
		if (!vim_ispathsep(src[0]) && --dstlen > 0)
		    *dst++ = '/';
		break;
	    }
	    if (p == homedir_env)
		break;
	    p = homedir_env;
	    len = envlen;
	}

	/* if (!one) skip to separator: space or comma */
	while (*src && (one || (*src != ',' && *src != ' ')) && --dstlen > 0)
	    *dst++ = *src++;
	/* skip separator */
	while ((*src == ' ' || *src == ',') && --dstlen > 0)
	    *dst++ = *src++;
    }
    /* if (dstlen == 0) out of space, what to do??? */

    *dst = NUL;
}

/*
 * Like home_replace, store the replaced string in allocated memory.
 * When something fails, NULL is returned.
 */
    char_u  *
home_replace_save(buf, src)
    buf_T	*buf;	/* when not NULL, check for help files */
    char_u	*src;	/* input file name */
{
    char_u	*dst;
    unsigned	len;

    len = 3;			/* space for "~/" and trailing NUL */
    if (src != NULL)		/* just in case */
	len += (unsigned)STRLEN(src);
    dst = alloc(len);
    if (dst != NULL)
	home_replace(buf, src, dst, len, TRUE);
    return dst;
}

/*
 * Compare two file names and return:
 * FPC_SAME   if they both exist and are the same file.
 * FPC_SAMEX  if they both don't exist and have the same file name.
 * FPC_DIFF   if they both exist and are different files.
 * FPC_NOTX   if they both don't exist.
 * FPC_DIFFX  if one of them doesn't exist.
 * For the first name environment variables are expanded
 */
    int
fullpathcmp(s1, s2, checkname)
    char_u *s1, *s2;
    int	    checkname;		/* when both don't exist, check file names */
{
#ifdef UNIX
    char_u	    exp1[MAXPATHL];
    char_u	    full1[MAXPATHL];
    char_u	    full2[MAXPATHL];
    struct stat	    st1, st2;
    int		    r1, r2;

    expand_env(s1, exp1, MAXPATHL);
    r1 = mch_stat((char *)exp1, &st1);
    r2 = mch_stat((char *)s2, &st2);
    if (r1 != 0 && r2 != 0)
    {
	/* if mch_stat() doesn't work, may compare the names */
	if (checkname)
	{
	    if (fnamecmp(exp1, s2) == 0)
		return FPC_SAMEX;
	    r1 = vim_FullName(exp1, full1, MAXPATHL, FALSE);
	    r2 = vim_FullName(s2, full2, MAXPATHL, FALSE);
	    if (r1 == OK && r2 == OK && fnamecmp(full1, full2) == 0)
		return FPC_SAMEX;
	}
	return FPC_NOTX;
    }
    if (r1 != 0 || r2 != 0)
	return FPC_DIFFX;
    if (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino)
	return FPC_SAME;
    return FPC_DIFF;
#else
    char_u  *exp1;		/* expanded s1 */
    char_u  *full1;		/* full path of s1 */
    char_u  *full2;		/* full path of s2 */
    int	    retval = FPC_DIFF;
    int	    r1, r2;

    /* allocate one buffer to store three paths (alloc()/free() is slow!) */
    if ((exp1 = alloc(MAXPATHL * 3)) != NULL)
    {
	full1 = exp1 + MAXPATHL;
	full2 = full1 + MAXPATHL;

	expand_env(s1, exp1, MAXPATHL);
	r1 = vim_FullName(exp1, full1, MAXPATHL, FALSE);
	r2 = vim_FullName(s2, full2, MAXPATHL, FALSE);

	/* If vim_FullName() fails, the file probably doesn't exist. */
	if (r1 != OK && r2 != OK)
	{
	    if (checkname && fnamecmp(exp1, s2) == 0)
		return FPC_SAMEX;
	    retval = FPC_NOTX;
	}
	else if (r1 != OK || r2 != OK)
	    retval = FPC_DIFFX;
	else if (fnamecmp(full1, full2))
	    retval = FPC_DIFF;
	else
	    retval = FPC_SAME;
    }
    vim_free(exp1);
    return retval;
#endif
}

/*
 * get the tail of a path: the file name.
 */
    char_u *
gettail(fname)
    char_u *fname;
{
    char_u  *p1, *p2;

    if (fname == NULL)
	return (char_u *)"";
    for (p1 = p2 = fname; *p2; )	/* find last part of path */
    {
	if (vim_ispathsep(*p2))
	    p1 = p2 + 1;
#ifdef FEAT_MBYTE
	if (has_mbyte)
	    p2 += (*mb_ptr2len_check)(p2);
	else
#endif
	    ++p2;
    }
    return p1;
}

/*
 * get the next path component (just after the next path separator).
 */
    char_u *
getnextcomp(fname)
    char_u *fname;
{
    while (*fname && !vim_ispathsep(*fname))
	++fname;
    if (*fname)
	++fname;
    return fname;
}

/*
 * Get a pointer to one character past the head of a path name.
 * Unix: after "/"; DOS: after "c:\"; Amiga: after "disk:/"; Mac: no head.
 * If there is no head, path is returned.
 */
    char_u *
get_past_head(path)
    char_u  *path;
{
    char_u  *retval;

#if defined(MSDOS) || defined(MSWIN) || defined(OS2)
    /* may skip "c:" */
    if (isalpha(path[0]) && path[1] == ':')
	retval = path + 2;
    else
	retval = path;
#else
# if defined(AMIGA)
    /* may skip "label:" */
    retval = vim_strchr(path, ':');
    if (retval == NULL)
	retval = path;
# else	/* Unix */
    retval = path;
# endif
#endif

    while (vim_ispathsep(*retval))
	++retval;

    return retval;
}

/*
 * return TRUE if 'c' is a path separator.
 */
    int
vim_ispathsep(c)
    int c;
{
#ifdef RISCOS
    return (c == '.' || c == ':');
#else
# ifdef UNIX
    return (c == '/');	    /* UNIX has ':' inside file names */
# else
#  ifdef BACKSLASH_IN_FILENAME
    return (c == ':' || c == '/' || c == '\\');
#  else
#   ifdef VMS
    /* server"user passwd"::device:[full.path.name]fname.extension;version" */
    return (c == ':' || c == '[' || c == ']' || c == '/'
	    || c == '<' || c == '>' || c == '"' );
#   else
#    ifdef COLON_AS_PATHSEP
    return (c == ':');
#    else		/* Amiga */
    return (c == ':' || c == '/');
#    endif
#   endif /* VMS */
#  endif
# endif
#endif /* RISC OS */
}

#if defined(FEAT_SEARCHPATH) || defined(PROTO)
/*
 * return TRUE if 'c' is a path list separator.
 */
    int
vim_ispathlistsep(c)
    int c;
{
#ifdef UNIX
    return (c == ':');
#else
    return (c == ';');	/* might not be rigth for every system... */
#endif
}
#endif

#if (defined(CASE_INSENSITIVE_FILENAME) && defined(BACKSLASH_IN_FILENAME)) \
	|| defined(PROTO)
/*
 * Versions of fnamecmp() and fnamencmp() that handle '/' and '\' equally.
 */
    int
vim_fnamecmp(x, y)
    char_u	*x, *y;
{
    return vim_fnamencmp(x, y, MAXPATHL);
}

    int
vim_fnamencmp(x, y, len)
    char_u	*x, *y;
    size_t	len;
{
    while (len > 0 && *x && *y)
    {
	if (TO_LOWER(*x) != TO_LOWER(*y)
		&& !(*x == '/' && *y == '\\')
		&& !(*x == '\\' && *y == '/'))
	    break;
	++x;
	++y;
	--len;
    }
    if (len <= 0)
	return 0;
    return (*x - *y);
}
#endif

/*
 * Concatenate file names fname1 and fname2 into allocated memory.
 * Only add a '/' or '\\' when 'sep' is TRUE and it is neccesary.
 */
    char_u  *
concat_fnames(fname1, fname2, sep)
    char_u  *fname1;
    char_u  *fname2;
    int	    sep;
{
    char_u  *dest;

    dest = alloc((unsigned)(STRLEN(fname1) + STRLEN(fname2) + 3));
    if (dest != NULL)
    {
	STRCPY(dest, fname1);
	if (sep)
	    add_pathsep(dest);
	STRCAT(dest, fname2);
    }
    return dest;
}

/*
 * Add a path separator to a file name, unless it already ends in a path
 * separator.
 */
    void
add_pathsep(p)
    char_u	*p;
{
    if (*p != NUL && !vim_ispathsep(*(p + STRLEN(p) - 1)))
	STRCAT(p, PATHSEPSTR);
}

/*
 * FullName_save - Make an allocated copy of a full file name.
 * Returns NULL when out of memory.
 */
    char_u  *
FullName_save(fname, force)
    char_u	*fname;
    int		force;	    /* force expansion, even when it already looks
			       like a full path name */
{
    char_u	*buf;
    char_u	*new_fname = NULL;

    if (fname == NULL)
	return NULL;

    buf = alloc((unsigned)MAXPATHL);
    if (buf != NULL)
    {
	if (vim_FullName(fname, buf, MAXPATHL, force) != FAIL)
	    new_fname = vim_strsave(buf);
	else
	    new_fname = vim_strsave(fname);
	vim_free(buf);
    }
    return new_fname;
}

#if defined(FEAT_CINDENT) || defined(FEAT_SYN_HL)

static char_u	*skip_string __ARGS((char_u *p));

/*
 * Find the start of a comment, not knowing if we are in a comment right now.
 * Search starts at w_cursor.lnum and goes backwards.
 */
    pos_T *
find_start_comment(ind_maxcomment)	    /* XXX */
    int		ind_maxcomment;
{
    pos_T	*pos;
    char_u	*line;
    char_u	*p;

    if ((pos = findmatchlimit(NULL, '*', FM_BACKWARD, ind_maxcomment)) == NULL)
	return NULL;

    /*
     * Check if the comment start we found is inside a string.
     */
    line = ml_get(pos->lnum);
    for (p = line; *p && (unsigned)(p - line) < pos->col; ++p)
	p = skip_string(p);
    if ((unsigned)(p - line) > pos->col)
	return NULL;
    return pos;
}

/*
 * Skip to the end of a "string" and a 'c' character.
 * If there is no string or character, return argument unmodified.
 */
    static char_u *
skip_string(p)
    char_u  *p;
{
    int	    i;

    /*
     * We loop, because strings may be concatenated: "date""time".
     */
    for ( ; ; ++p)
    {
	if (p[0] == '\'')		    /* 'c' or '\n' or '\000' */
	{
	    if (!p[1])			    /* ' at end of line */
		break;
	    i = 2;
	    if (p[1] == '\\')		    /* '\n' or '\000' */
	    {
		++i;
		while (isdigit(p[i - 1]))   /* '\000' */
		    ++i;
	    }
	    if (p[i] == '\'')		    /* check for trailing ' */
	    {
		p += i;
		continue;
	    }
	}
	else if (p[0] == '"')		    /* start of string */
	{
	    for (++p; p[0]; ++p)
	    {
		if (p[0] == '\\' && p[1] != NUL)
		    ++p;
		else if (p[0] == '"')	    /* end of string */
		    break;
	    }
	    if (p[0] == '"')
		continue;
	}
	break;				    /* no string found */
    }
    if (!*p)
	--p;				    /* backup from NUL */
    return p;
}
#endif /* FEAT_CINDENT || FEAT_SYN_HL */

#ifdef FEAT_CINDENT

/*
 * Do C or expression indenting on the current line.
 */
    void
do_c_expr_indent()
{
# ifdef FEAT_EVAL
    if (*curbuf->b_p_inde != NUL)
	fixthisline(get_expr_indent);
    else
# endif
	fixthisline(get_c_indent);
}

/*
 * Functions for C-indenting.
 * Most of this originally comes from Eric Fischer.
 */
/*
 * Below "XXX" means that this function may unlock the current line.
 */

static char_u	*cin_skipcomment __ARGS((char_u *));
static int	cin_nocode __ARGS((char_u *));
static int	cin_islabel_skip __ARGS((char_u **));
static int	cin_isdefault __ARGS((char_u *));
static char_u	*after_label __ARGS((char_u *l));
static int	get_indent_nolabel __ARGS((linenr_T lnum));
static int	skip_label __ARGS((linenr_T, char_u **pp, int ind_maxcomment));
static int	cin_ispreproc __ARGS((char_u *));
static int	cin_iscomment __ARGS((char_u *));
static int	cin_isterminated __ARGS((char_u *, int));
static int	cin_isfuncdecl __ARGS((char_u *));
static int	cin_isif __ARGS((char_u *));
static int	cin_iselse __ARGS((char_u *));
static int	cin_isdo __ARGS((char_u *));
static int	cin_iswhileofdo __ARGS((char_u *, linenr_T, int));
static int	cin_ends_in __ARGS((char_u *, char_u *));
static int	cin_skip2pos __ARGS((pos_T *trypos));
static pos_T	*find_start_brace __ARGS((int));
static pos_T	*find_match_paren __ARGS((int, int));
static int	find_last_paren __ARGS((char_u *l));
static int	find_match __ARGS((int lookfor, linenr_T ourscope, int ind_maxparen, int ind_maxcomment));

/*
 * Skip over white space and C comments within the line.
 */
    static char_u *
cin_skipcomment(s)
    char_u	*s;
{
    while (*s)
    {
	s = skipwhite(s);
	if (*s != '/')
	    break;
	++s;
	if (*s == '/')		/* slash-slash comment continues till eol */
	{
	    s += STRLEN(s);
	    break;
	}
	if (*s != '*')
	    break;
	for (++s; *s; ++s)	/* skip slash-star comment */
	    if (s[0] == '*' && s[1] == '/')
	    {
		s += 2;
		break;
	    }
    }
    return s;
}

/*
 * Return TRUE if there there is no code at *s.  White space and comments are
 * not considered code.
 */
    static int
cin_nocode(s)
    char_u	*s;
{
    return *cin_skipcomment(s) == NUL;
}

/*
 * Check if string matches "label:"; move to character after ':' if true.
 */
    static int
cin_islabel_skip(s)
    char_u	**s;
{
    if (!vim_isIDc(**s))	    /* need at least one ID character */
	return FALSE;

    while (vim_isIDc(**s))
	(*s)++;

    *s = cin_skipcomment(*s);

    /* "::" is not a label, it's C++ */
    return (**s == ':' && *++*s != ':');
}

/*
 * Recognize a label: "label:".
 * Note: curwin->w_cursor must be where we are looking for the label.
 */
    int
cin_islabel(ind_maxcomment)		/* XXX */
    int		ind_maxcomment;
{
    char_u	*s;

    s = cin_skipcomment(ml_get_curline());

    /*
     * Exclude "default" from labels, since it should be indented
     * like a switch label.  Same for C++ scope declarations.
     */
    if (cin_isdefault(s))
	return FALSE;
    if (cin_isscopedecl(s))
	return FALSE;

    if (cin_islabel_skip(&s))
    {
	/*
	 * Only accept a label if the previous line is terminated or is a case
	 * label.
	 */
	pos_T	cursor_save;
	pos_T	*trypos;
	char_u	*line;

	cursor_save = curwin->w_cursor;
	while (curwin->w_cursor.lnum > 1)
	{
	    --curwin->w_cursor.lnum;

	    /*
	     * If we're in a comment now, skip to the start of the comment.
	     */
	    curwin->w_cursor.col = 0;
	    if ((trypos = find_start_comment(ind_maxcomment)) != NULL) /* XXX */
		curwin->w_cursor = *trypos;

	    line = ml_get_curline();
	    if (cin_ispreproc(line))	/* ignore #defines, #if, etc. */
		continue;
	    if (*(line = cin_skipcomment(line)) == NUL)
		continue;

	    curwin->w_cursor = cursor_save;
	    if (cin_isterminated(line, TRUE)
		    || cin_isscopedecl(line)
		    || cin_iscase(line)
		    || (cin_islabel_skip(&line) && cin_nocode(line)))
		return TRUE;
	    return FALSE;
	}
	curwin->w_cursor = cursor_save;
	return TRUE;		/* label at start of file??? */
    }
    return FALSE;
}

/*
 * Recognize a switch label: "case .*:" or "default:".
 */
     int
cin_iscase(s)
    char_u *s;
{
    s = cin_skipcomment(s);
    if (STRNCMP(s, "case", 4) == 0 && !vim_isIDc(s[4]))
    {
	for (s += 4; *s; ++s)
	{
	    s = cin_skipcomment(s);
	    if (*s == ':')
	    {
		if (s[1] == ':')	/* skip over "::" for C++ */
		    ++s;
		else
		    return TRUE;
	    }
	    if (*s == '\'' && s[1] && s[2] == '\'')
		s += 2;			/* skip over '.' */
	    else if (*s == '/' && (s[1] == '*' || s[1] == '/'))
		return FALSE;		/* stop at comment */
	    else if (*s == '"')
		return FALSE;		/* stop at string */
	}
	return FALSE;
    }

    if (cin_isdefault(s))
	return TRUE;
    return FALSE;
}

/*
 * Recognize a "default" switch label.
 */
    static int
cin_isdefault(s)
    char_u  *s;
{
    return (STRNCMP(s, "default", 7) == 0
	    && *(s = cin_skipcomment(s + 7)) == ':'
	    && s[1] != ':');
}

/*
 * Recognize a "public/private/proctected" scope declaration label.
 */
    int
cin_isscopedecl(s)
    char_u	*s;
{
    int		i;

    s = cin_skipcomment(s);
    if (STRNCMP(s, "public", 6) == 0)
	i = 6;
    else if (STRNCMP(s, "protected", 9) == 0)
	i = 9;
    else if (STRNCMP(s, "private", 7) == 0)
	i = 7;
    else
	return FALSE;
    return (*(s = cin_skipcomment(s + i)) == ':' && s[1] != ':');
}

/*
 * Return a pointer to the first non-empty non-comment character after a ':'.
 * Return NULL if not found.
 *	  case 234:    a = b;
 *		       ^
 */
    static char_u *
after_label(l)
    char_u  *l;
{
    for ( ; *l; ++l)
    {
	if (*l == ':')
	{
	    if (l[1] == ':')	    /* skip over "::" for C++ */
		++l;
	    else if (!cin_iscase(l + 1))
		break;
	}
	else if (*l == '\'' && l[1] && l[2] == '\'')
	    l += 2;		    /* skip over 'x' */
    }
    if (*l == NUL)
	return NULL;
    l = cin_skipcomment(l + 1);
    if (*l == NUL)
	return NULL;
    return l;
}

/*
 * Get indent of line "lnum", skipping a label.
 * Return 0 if there is nothing after the label.
 */
    static int
get_indent_nolabel(lnum)		/* XXX */
    linenr_T	lnum;
{
    char_u	*l;
    pos_T	fp;
    colnr_T	col;
    char_u	*p;

    l = ml_get(lnum);
    p = after_label(l);
    if (p == NULL)
	return 0;

    fp.col = (colnr_T)(p - l);
    fp.lnum = lnum;
    getvcol(curwin, &fp, &col, NULL, NULL);
    return (int)col;
}

/*
 * Find indent for line "lnum", ignoring any case or jump label.
 * Also return a pointer to the text (after the label).
 *   label:	if (asdf && asdfasdf)
 *		^
 */
    static int
skip_label(lnum, pp, ind_maxcomment)
    linenr_T	lnum;
    char_u	**pp;
    int		ind_maxcomment;
{
    char_u	*l;
    int		amount;
    pos_T	cursor_save;

    cursor_save = curwin->w_cursor;
    curwin->w_cursor.lnum = lnum;
    l = ml_get_curline();
				    /* XXX */
    if (cin_iscase(l) || cin_isscopedecl(l) || cin_islabel(ind_maxcomment))
    {
	amount = get_indent_nolabel(lnum);
	l = after_label(ml_get_curline());
	if (l == NULL)		/* just in case */
	    l = ml_get_curline();
    }
    else
    {
	amount = get_indent();
	l = ml_get_curline();
    }
    *pp = l;

    curwin->w_cursor = cursor_save;
    return amount;
}

/*
 * Recognize a preprocessor statement: Any line that starts with '#'.
 */
    static int
cin_ispreproc(s)
    char_u *s;
{
    s = skipwhite(s);
    if (*s == '#')
	return TRUE;
    return FALSE;
}

/*
 * Recognize the start of a C or C++ comment.
 */
    static int
cin_iscomment(p)
    char_u  *p;
{
    return (p[0] == '/' && (p[1] == '*' || p[1] == '/'));
}

/*
 * Recognize a line that starts with '{' or '}', or ends with ';', '{' or '}'.
 * Don't consider "} else" a terminated line.
 * Also consider a line terminated if it ends in ','.  This is not 100%
 * correct, but this mostly means we are in initializations and then it's OK.
 */
    static int
cin_isterminated(s, incl_open)
    char_u	*s;
    int		incl_open;	/* include '{' at the end as terminator */
{
    s = cin_skipcomment(s);

    if (*s == '{' || (*s == '}' && !cin_iselse(s)))
	return TRUE;

    while (*s)
    {
	/* skip over comments, "" strings and 'c'haracters */
	s = skip_string(cin_skipcomment(s));
	if ((*s == ';' || (incl_open && *s == '{') || *s == '}' || *s == ',')
						     && cin_nocode(s + 1))
	    return TRUE;
	if (*s)
	    s++;
    }
    return FALSE;
}

/*
 * Recognize the basic picture of a function declaration -- it needs to
 * have an open paren somewhere and a close paren at the end of the line and
 * no semicolons anywhere.
 */
    static int
cin_isfuncdecl(s)
    char_u *s;
{
    while (*s && *s != '(' && *s != ';' && *s != '\'' && *s != '"')
    {
	if (cin_iscomment(s))	/* ignore comments */
	    s = cin_skipcomment(s);
	else
	    ++s;
    }
    if (*s != '(')
	return FALSE;		/* ';', ' or "  before any () or no '(' */

    while (*s && *s != ';' && *s != '\'' && *s != '"')
    {
	if (*s == ')' && cin_nocode(s + 1))
	    return TRUE;
	if (cin_iscomment(s))	/* ignore comments */
	    s = cin_skipcomment(s);
	else
	    ++s;
    }
    return FALSE;
}

    static int
cin_isif(p)
    char_u  *p;
{
    return (STRNCMP(p, "if", 2) == 0 && !vim_isIDc(p[2]));
}

    static int
cin_iselse(p)
    char_u  *p;
{
    if (*p == '}')	    /* accept "} else" */
	p = cin_skipcomment(p + 1);
    return (STRNCMP(p, "else", 4) == 0 && !vim_isIDc(p[4]));
}

    static int
cin_isdo(p)
    char_u  *p;
{
    return (STRNCMP(p, "do", 2) == 0 && !vim_isIDc(p[2]));
}

/*
 * Check if this is a "while" that should have a matching "do".
 * We only accept a "while (condition) ;", with only white space between the
 * ')' and ';'. The condition may be spread over several lines.
 */
    static int
cin_iswhileofdo(p, lnum, ind_maxparen)	    /* XXX */
    char_u	*p;
    linenr_T	lnum;
    int		ind_maxparen;
{
    pos_T	cursor_save;
    pos_T	*trypos;
    int		retval = FALSE;

    p = cin_skipcomment(p);
    if (*p == '}')		/* accept "} while (cond);" */
	p = cin_skipcomment(p + 1);
    if (STRNCMP(p, "while", 5) == 0 && !vim_isIDc(p[5]))
    {
	cursor_save = curwin->w_cursor;
	curwin->w_cursor.lnum = lnum;
	curwin->w_cursor.col = 0;
	p = ml_get_curline();
	while (*p && *p != 'w')	/* skip any '}', until the 'w' of the "while" */
	{
	    ++p;
	    ++curwin->w_cursor.col;
	}
	if ((trypos = findmatchlimit(NULL, 0, 0, ind_maxparen)) != NULL
		&& *cin_skipcomment(ml_get_pos(trypos) + 1) == ';')
	    retval = TRUE;
	curwin->w_cursor = cursor_save;
    }
    return retval;
}

/*
 * Return TRUE if string "s" ends with the string "find", possibly followed by
 * white space and comments.  Skip strings and comments.
 */
    static int
cin_ends_in(s, find)
    char_u	*s;
    char_u	*find;
{
    char_u	*p = s;
    char_u	*r;
    int		len = (int)STRLEN(find);

    while (*p != NUL)
    {
	p = cin_skipcomment(p);
	if (STRNCMP(p, find, len) == 0)
	{
	    r = skipwhite(p + len);
	    if (*r == NUL || cin_iscomment(r))
		return TRUE;
	}
	if (*p != NUL)
	    ++p;
    }
    return FALSE;
}

/*
 * Skip strings, chars and comments until at or past "trypos".
 * Return the column found.
 */
    static int
cin_skip2pos(trypos)
    pos_T	*trypos;
{
    char_u	*line;
    char_u	*p;

    p = line = ml_get(trypos->lnum);
    while (*p && (colnr_T)(p - line) < trypos->col)
    {
	if (cin_iscomment(p))
	    p = cin_skipcomment(p);
	else
	{
	    p = skip_string(p);
	    ++p;
	}
    }
    return (int)(p - line);
}

/*
 * Find the '{' at the start of the block we are in.
 * Return NULL if no match found.
 * Ignore a '{' that is in a comment, makes indenting the next three lines
 * work. */
/* foo()    */
/* {	    */
/* }	    */

    static pos_T *
find_start_brace(ind_maxcomment)	    /* XXX */
    int		ind_maxcomment;
{
    pos_T	cursor_save;
    pos_T	*trypos;
    pos_T	*pos;
    static pos_T	pos_copy;

    cursor_save = curwin->w_cursor;
    while ((trypos = findmatchlimit(NULL, '{', FM_BLOCKSTOP, 0)) != NULL)
    {
	pos_copy = *trypos;	/* copy pos_T, next findmatch will change it */
	trypos = &pos_copy;
	curwin->w_cursor = *trypos;
	pos = NULL;
	/* ignore the { if it's in a // comment */
	if ((colnr_T)cin_skip2pos(trypos) == trypos->col
		&& (pos = find_start_comment(ind_maxcomment)) == NULL) /* XXX */
	    break;
	if (pos != NULL)
	    curwin->w_cursor.lnum = pos->lnum;
    }
    curwin->w_cursor = cursor_save;
    return trypos;
}

/*
 * Find the matching '(', failing if it is in a comment.
 * Return NULL of no match found.
 */
    static pos_T *
find_match_paren(ind_maxparen, ind_maxcomment)	    /* XXX */
    int		ind_maxparen;
    int		ind_maxcomment;
{
    pos_T	cursor_save;
    pos_T	*trypos;
    static pos_T	pos_copy;

    cursor_save = curwin->w_cursor;
    if ((trypos = findmatchlimit(NULL, '(', 0, ind_maxparen)) != NULL)
    {
	/* check if the ( is in a // comment */
	if ((colnr_T)cin_skip2pos(trypos) > trypos->col)
	    trypos = NULL;
	else
	{
	    pos_copy = *trypos;	    /* copy trypos, findmatch will change it */
	    trypos = &pos_copy;
	    curwin->w_cursor = *trypos;
	    if (find_start_comment(ind_maxcomment) != NULL) /* XXX */
		trypos = NULL;
	}
    }
    curwin->w_cursor = cursor_save;
    return trypos;
}

/*
 * Set w_cursor.col to the column number of the last ')' in line "l".
 */
    static int
find_last_paren(l)
    char_u *l;
{
    int	    i;
    int	    retval = FALSE;

    curwin->w_cursor.col = 0;		    /* default is start of line */

    for (i = 0; l[i]; i++)
    {
	i = (int)(skip_string(l + i) - l);    /* ignore parens in quotes */
	if (l[i] == ')')
	{
	    curwin->w_cursor.col = i;
	    retval = TRUE;
	}
    }
    return retval;
}

    int
get_c_indent()
{
    /*
     * spaces from a block's opening brace the prevailing indent for that
     * block should be
     */
    int ind_level = curbuf->b_p_sw;

    /*
     * spaces from the edge of the line an open brace that's at the end of a
     * line is imagined to be.
     */
    int ind_open_imag = 0;

    /*
     * spaces from the prevailing indent for a line that is not precededof by
     * an opening brace.
     */
    int ind_no_brace = 0;

    /*
     * column where the first { of a function should be located }
     */
    int ind_first_open = 0;

    /*
     * spaces from the prevailing indent a leftmost open brace should be
     * located
     */
    int ind_open_extra = 0;

    /*
     * spaces from the matching open brace (real location for one at the left
     * edge; imaginary location from one that ends a line) the matching close
     * brace should be located
     */
    int ind_close_extra = 0;

    /*
     * spaces from the edge of the line an open brace sitting in the leftmost
     * column is imagined to be
     */
    int ind_open_left_imag = 0;

    /*
     * spaces from the switch() indent a "case xx" label should be located
     */
    int ind_case = curbuf->b_p_sw;

    /*
     * spaces from the "case xx:" code after a switch() should be located
     */
    int ind_case_code = curbuf->b_p_sw;

    /*
     * spaces from the class declaration indent a scope declaration label
     * should be located
     */
    int ind_scopedecl = curbuf->b_p_sw;

    /*
     * spaces from the scope declaration label code should be located
     */
    int ind_scopedecl_code = curbuf->b_p_sw;

    /*
     * amount K&R-style parameters should be indented
     */
    int ind_param = curbuf->b_p_sw;

    /*
     * amount a function type spec should be indented
     */
    int ind_func_type = curbuf->b_p_sw;

    /*
     * additional spaces beyond the prevailing indent a continuation line
     * should be located
     */
    int ind_continuation = curbuf->b_p_sw;

    /*
     * spaces from the indent of the line with an unclosed parentheses
     */
    int ind_unclosed = curbuf->b_p_sw * 2;

    /*
     * spaces from the indent of the line with an unclosed parentheses, which
     * itself is also unclosed
     */
    int ind_unclosed2 = curbuf->b_p_sw;

    /*
     * spaces from the comment opener when there is nothing after it.
     */
    int ind_in_comment = 3;

    /*
     * max lines to search for an open paren
     */
    int ind_maxparen = 20;

    /*
     * max lines to search for an open comment
     */
    int ind_maxcomment = 70;

    /*
     * handle braces for java code
     */
    int	ind_java = 0;

    /*
     * handle blocked cases correctly
     */
    int ind_keep_case_label = 0;

    pos_T	cur_curpos;
    int		amount;
    int		scope_amount;
    int		cur_amount;
    colnr_T	col;
    char_u	*theline;
    char_u	*linecopy;
    pos_T	*trypos;
    pos_T	*tryposBrace = NULL;
    pos_T	our_paren_pos;
    char_u	*start;
    int		start_brace;
#define BRACE_IN_COL0		1	    /* '{' is in comumn 0 */
#define BRACE_AT_START		2	    /* '{' is at start of line */
#define BRACE_AT_END		3	    /* '{' is at end of line */
    linenr_T	ourscope;
    char_u	*l;
    char_u	*look;
    int		lookfor;
#define LOOKFOR_IF		1
#define LOOKFOR_DO		2
#define LOOKFOR_CASE		3
#define LOOKFOR_ANY		4
#define LOOKFOR_TERM		5
#define LOOKFOR_UNTERM		6
#define LOOKFOR_SCOPEDECL	7
    int		whilelevel;
    linenr_T	lnum;
    char_u	*options;
    int		fraction = 0;	    /* init for GCC */
    int		divider;
    int		n;
    int		iscase;

    for (options = curbuf->b_p_cino; *options; )
    {
	l = options++;
	if (*options == '-')
	    ++options;
	n = getdigits(&options);
	divider = 0;
	if (*options == '.')	    /* ".5s" means a fraction */
	{
	    fraction = atol((char *)++options);
	    while (isdigit(*options))
	    {
		++options;
		if (divider)
		    divider *= 10;
		else
		    divider = 10;
	    }
	}
	if (*options == 's')	    /* "2s" means two times 'shiftwidth' */
	{
	    if (n == 0 && fraction == 0)
		n = curbuf->b_p_sw;	/* just "s" is one 'shiftwidth' */
	    else
	    {
		n *= curbuf->b_p_sw;
		if (divider)
		    n += (curbuf->b_p_sw * fraction + divider / 2) / divider;
	    }
	    ++options;
	}
	if (l[1] == '-')
	    n = -n;
	/* When adding an entry here, also update the default 'cinoptions' in
	 * change.txt, and add explanation for it! */
	switch (*l)
	{
	    case '>': ind_level = n; break;
	    case 'e': ind_open_imag = n; break;
	    case 'n': ind_no_brace = n; break;
	    case 'f': ind_first_open = n; break;
	    case '{': ind_open_extra = n; break;
	    case '}': ind_close_extra = n; break;
	    case '^': ind_open_left_imag = n; break;
	    case ':': ind_case = n; break;
	    case '=': ind_case_code = n; break;
	    case 'p': ind_param = n; break;
	    case 't': ind_func_type = n; break;
	    case 'c': ind_in_comment = n; break;
	    case '+': ind_continuation = n; break;
	    case '(': ind_unclosed = n; break;
	    case 'u': ind_unclosed2 = n; break;
	    case ')': ind_maxparen = n; break;
	    case '*': ind_maxcomment = n; break;
	    case 'g': ind_scopedecl = n; break;
	    case 'h': ind_scopedecl_code = n; break;
	    case 'j': ind_java = n; break;
	    case 'l': ind_keep_case_label = n; break;
	}
    }

    /* remember where the cursor was when we started */

    cur_curpos = curwin->w_cursor;

    /* get the current contents of the line.
     * This is required, because only the most recent line obtained with
     * ml_get is valid! */

    linecopy = vim_strsave(ml_get(cur_curpos.lnum));
    if (linecopy == NULL)
	return 0;

    /*
     * In insert mode and the cursor is on a ')' truncate the line at the
     * cursor position.  We don't want to line up with the matching '(' when
     * inserting new stuff.
     */
    if ((State & INSERT) && linecopy[curwin->w_cursor.col] == ')')
	linecopy[curwin->w_cursor.col] = NUL;

    theline = skipwhite(linecopy);

    /* move the cursor to the start of the line */

    curwin->w_cursor.col = 0;

    /*
     * #defines and so on always go at the left when included in 'cinkeys'.
     */
    if (*theline == '#' && (*linecopy == '#' || in_cinkeys('#', ' ', TRUE)))
    {
	amount = 0;
    }

    /*
     * Is it a non-case label?	Then that goes at the left margin too.
     */
    else if (cin_islabel(ind_maxcomment))	    /* XXX */
    {
	amount = 0;
    }

    /*
     * If we're inside a comment and not looking at the start of the
     * comment, try using the 'comments' option.
     */
    else if (!cin_iscomment(theline)
	    && (trypos = find_start_comment(ind_maxcomment)) != NULL) /* XXX */
    {
	int	lead_start_len = 2;
	char_u	lead_middle[COM_MAX_LEN];	/* middle-comment string */
	char_u	lead_end[COM_MAX_LEN];		/* end-comment string */
	char_u	*p;
	int	start_align = 0;
	int	start_off = 0;
	int	done = FALSE;

	/* find how indented the line beginning the comment is */
	getvcol(curwin, trypos, &col, NULL, NULL);
	amount = col;

	p = curbuf->b_p_com;
	while (*p != NUL)
	{
	    int	align = 0;
	    int	off = 0;
	    int what = 0;

	    while (*p != NUL && *p != ':')
	    {
		if (*p == COM_START || *p == COM_END || *p == COM_MIDDLE)
		    what = *p++;
		else if (*p == COM_LEFT || *p == COM_RIGHT)
		    align = *p++;
		else if (isdigit(*p) || *p == '-')
		    off = getdigits(&p);
		else
		    ++p;
	    }

	    if (*p == ':')
		++p;
	    (void)copy_option_part(&p, lead_end, COM_MAX_LEN, ",");
	    if (what == COM_START)
	    {
		lead_start_len = (int)STRLEN(lead_end);
		start_off = off;
		start_align = align;
	    }
	    else if (what == COM_MIDDLE)
	    {
		STRCPY(lead_middle, lead_end);
	    }
	    else if (what == COM_END)
	    {
		/* If our line starts with the middle comment string, line it
		 * up with the comment opener per the 'comments' option. */
		if (STRNCMP(theline, lead_middle, STRLEN(lead_middle)) == 0
			&& STRNCMP(theline, lead_end, STRLEN(lead_end)) != 0)
		{
		    if (start_off != 0)
			amount += start_off;
		    else if (start_align == COM_RIGHT)
			amount += lead_start_len - (int)STRLEN(lead_middle);
		    done = TRUE;
		    break;
		}

		/* If our line starts with the end comment string, line it up
		 * with the middle comment */
		if (STRNCMP(theline, lead_middle, STRLEN(lead_middle)) != 0
			&& STRNCMP(theline, lead_end, STRLEN(lead_end)) == 0)
		{
		    amount = get_indent_lnum(curwin->w_cursor.lnum - 1);
								     /* XXX */
		    if (off != 0)
			amount += off;
		    else if (align == COM_RIGHT)
			amount += lead_start_len - (int)STRLEN(lead_middle);
		    done = TRUE;
		    break;
		}
	    }
	}

	/* If our line starts with an asterisk, line up with the
	 * asterisk in the comment opener; otherwise, line up
	 * with the first character of the comment text.
	 */
	if (done)
	    ;
	else if (theline[0] == '*')
	    amount += 1;
	else
	{
	    /*
	     * If we are more than one line away from the comment opener, take
	     * the indent of the previous non-empty line.
	     * If we are just below the comment opener and there are any
	     * white characters after it line up with the text after it.
	     * up with them; otherwise, just use a single space.
	     */
	    amount = -1;
	    for (lnum = cur_curpos.lnum - 1; lnum > trypos->lnum; --lnum)
	    {
		if (linewhite(lnum))		    /* skip blank lines */
		    continue;
		amount = get_indent_lnum(lnum);	    /* XXX */
		break;
	    }
	    if (amount == -1)			    /* use the comment opener */
	    {
		start = ml_get(trypos->lnum);
		look = start + trypos->col + 2;	    /* skip / and * */
		if (*look != NUL)		    /* if something after it */
		    trypos->col = (colnr_T)(skipwhite(look) - start);
		getvcol(curwin, trypos, &col, NULL, NULL);
		amount = col;
		if (*look == NUL)
		    amount += ind_in_comment;
	    }
	}
    }

    /*
     * Are we inside parentheses or braces?
     */						    /* XXX */
    else if (((trypos = find_match_paren(ind_maxparen, ind_maxcomment)) != NULL
		&& ind_java == 0)
	    || (tryposBrace = find_start_brace(ind_maxcomment)) != NULL
	    || trypos != NULL)
    {
      if (trypos != NULL && tryposBrace != NULL)
      {
	  /* Both an unmatched '(' and '{' is found.  Use the one which is
	   * closer to the current cursor position, set the other to NULL. */
	  if (trypos->lnum != tryposBrace->lnum
		  ? trypos->lnum < tryposBrace->lnum
		  : trypos->col < tryposBrace->col)
	      trypos = NULL;
	  else
	      tryposBrace = NULL;
      }

      if (trypos != NULL)
      {
	/*
	 * If the matching paren is more than one line away, use the indent of
	 * a previous non-empty line that matches the same paren.
	 */
	amount = -1;
	our_paren_pos = *trypos;
	if (theline[0] != ')')
	{
	    for (lnum = cur_curpos.lnum - 1; lnum > our_paren_pos.lnum; --lnum)
	    {
		l = skipwhite(ml_get(lnum));
		if (cin_nocode(l))	    /* skip comment lines */
		    continue;
		if (cin_ispreproc(l))	    /* ignore #defines, #if, etc. */
		    continue;
		curwin->w_cursor.lnum = lnum;

		/* Skip a comment. XXX */
		if ((trypos = find_start_comment(ind_maxcomment)) != NULL)
		{
		    lnum = trypos->lnum + 1;
		    continue;
		}

		/* XXX */
		if ((trypos = find_match_paren(ind_maxparen,
						   ind_maxcomment)) != NULL &&
					 trypos->lnum == our_paren_pos.lnum &&
					     trypos->col == our_paren_pos.col)
		{
		    amount = get_indent_lnum(lnum);	/* XXX */
		    break;
		}
	    }
	}

	/*
	 * Line up with line where the matching paren is. XXX
	 * If the line starts with a '(' or the indent for unclosed
	 * parentheses is zero, line up with the unclosed parentheses.
	 */
	if (amount == -1)
	{
	    amount = skip_label(our_paren_pos.lnum, &look, ind_maxcomment);
	    cur_amount = MAXCOL;
	    if (theline[0] == ')' || ind_unclosed == 0
						   || *skipwhite(look) == '(')
	    {
		/*
		 * If we're looking at a close paren, line up right there;
		 * otherwise, line up with the next non-white character.
		 */
		if (theline[0] != ')')
		{
		    col = our_paren_pos.col + 1;
		    look = ml_get(our_paren_pos.lnum);
		    while (vim_iswhite(look[col]))
			col++;
		    if (look[col] != NUL)	/* In case of trailing space */
			our_paren_pos.col = col;
		    else
			our_paren_pos.col++;
		}

		/*
		 * Find how indented the paren is, or the character after it
		 * if we did the above "if".
		 */
		getvcol(curwin, &our_paren_pos, &col, NULL, NULL);
		cur_amount = col;
	    }

	    if (ind_unclosed == 0 || *skipwhite(look) == '(')
		amount = cur_amount;
	    else
	    {
		/* add ind_unclosed2 for each '(' before our matching one */
		col = our_paren_pos.col;
		while (our_paren_pos.col > 0)
		{
		    --our_paren_pos.col;
		    switch (*ml_get_pos(&our_paren_pos))
		    {
			case '(': amount += ind_unclosed2;
				  col = our_paren_pos.col;
				  break;
			case ')': amount -= ind_unclosed2;
				  col = MAXCOL;
				  break;
		    }
		}

		/* Use ind_unclosed once, when the first '(' is not inside
		 * braces */
		if (col == MAXCOL)
		    amount += ind_unclosed;
		else
		{
		    curwin->w_cursor.lnum = our_paren_pos.lnum;
		    curwin->w_cursor.col = col;
		    if ((trypos = find_match_paren(ind_maxparen,
						     ind_maxcomment)) != NULL)
			amount += ind_unclosed2;
		    else
			amount += ind_unclosed;
		}
		/*
		 * For a line starting with ')' use the minimum of the two
		 * positions, to avoid giving it more indent than the previous
		 * lines:
		 *  func_long_name(		    if (x
		 *	arg				    && yy
		 *	)         ^ not here	       )    ^ not here
		 */
		if (cur_amount < amount)
		    amount = cur_amount;
	    }
	}
      }

      /*
       * Are we at least inside braces, then?
       */
      else
      {
	trypos = tryposBrace;

	ourscope = trypos->lnum;
	start = ml_get(ourscope);

	/*
	 * Now figure out how indented the line is in general.
	 * If the brace was at the start of the line, we use that;
	 * otherwise, check out the indentation of the line as
	 * a whole and then add the "imaginary indent" to that.
	 */
	look = skipwhite(start);
	if (*look == '{')
	{
	    getvcol(curwin, trypos, &col, NULL, NULL);
	    amount = col;
	    if (*start == '{')
		start_brace = BRACE_IN_COL0;
	    else
		start_brace = BRACE_AT_START;
	}
	else
	{
	    /*
	     * that opening brace might have been on a continuation
	     * line.  if so, find the start of the line.
	     */
	    curwin->w_cursor.lnum = ourscope;

	    /*
	     * position the cursor over the rightmost paren, so that
	     * matching it will take us back to the start of the line.
	     */
	    lnum = ourscope;
	    if (find_last_paren(start) &&
		    (trypos = find_match_paren(ind_maxparen,
						     ind_maxcomment)) != NULL)
		lnum = trypos->lnum;

	    /*
	     * It could have been something like
	     *	   case 1: if (asdf &&
	     *			ldfd) {
	     *		    }
	     */
	    if (ind_keep_case_label && cin_iscase(skipwhite(ml_get_curline())))
		amount = get_indent();
	    else
		amount = skip_label(lnum, &l, ind_maxcomment);

	    start_brace = BRACE_AT_END;
	}

	/*
	 * if we're looking at a closing brace, that's where
	 * we want to be.  otherwise, add the amount of room
	 * that an indent is supposed to be.
	 */
	if (theline[0] == '}')
	{
	    /*
	     * they may want closing braces to line up with something
	     * other than the open brace.  indulge them, if so.
	     */
	    amount += ind_close_extra;
	}
	else
	{
	    /*
	     * If we're looking at an "else", try to find an "if"
	     * to match it with.
	     * If we're looking at a "while", try to find a "do"
	     * to match it with.
	     */
	    lookfor = 0;
	    if (cin_iselse(theline))
		lookfor = LOOKFOR_IF;
	    else if (cin_iswhileofdo(theline, cur_curpos.lnum, ind_maxparen))
								    /* XXX */
		lookfor = LOOKFOR_DO;
	    if (lookfor)
	    {
		curwin->w_cursor.lnum = cur_curpos.lnum;
		if (find_match(lookfor, ourscope, ind_maxparen,
							ind_maxcomment) == OK)
		{
		    amount = get_indent();	/* XXX */
		    goto theend;
		}
	    }

	    /*
	     * We get here if we are not on an "while-of-do" or "else" (or
	     * failed to find a matching "if").
	     * Search backwards for something to line up with.
	     * First set amount for when we don't find anything.
	     */

	    /*
	     * if the '{' is  _really_ at the left margin, use the imaginary
	     * location of a left-margin brace.  Otherwise, correct the
	     * location for ind_open_extra.
	     */

	    if (start_brace == BRACE_IN_COL0)	    /* '{' is in column 0 */
	    {
		amount = ind_open_left_imag;
	    }
	    else
	    {
		if (start_brace == BRACE_AT_END)    /* '{' is at end of line */
		    amount += ind_open_imag;
		else
		{
		    /* Compensate for adding ind_open_extra later. */
		    amount -= ind_open_extra;
		    if (amount < 0)
			amount = 0;
		}
	    }

	    if (cin_iscase(theline))	/* it's a switch() label */
	    {
		lookfor = LOOKFOR_CASE;	/* find a previous switch() label */
		amount += ind_case;
	    }
	    else if (cin_isscopedecl(theline))	/* private:, ... */
	    {
		lookfor = LOOKFOR_SCOPEDECL;	/* class decl is this block */
		amount += ind_scopedecl;
	    }
	    else
	    {
		lookfor = LOOKFOR_ANY;
		amount += ind_level;	/* ind_level from start of block */
	    }
	    scope_amount = amount;
	    whilelevel = 0;

	    /*
	     * Search backwards.  If we find something we recognize, line up
	     * with that.
	     *
	     * if we're looking at an open brace, indent
	     * the usual amount relative to the conditional
	     * that opens the block.
	     */
	    curwin->w_cursor = cur_curpos;
	    for (;;)
	    {
		curwin->w_cursor.lnum--;
		curwin->w_cursor.col = 0;

		/*
		 * If we went all the way back to the start of our scope, line
		 * up with it.
		 */
		if (curwin->w_cursor.lnum <= ourscope)
		{
		    if (lookfor == LOOKFOR_UNTERM)
			amount += ind_continuation;
		    else if (lookfor != LOOKFOR_TERM)
		    {
			amount = scope_amount;
			if (theline[0] == '{')
			    amount += ind_open_extra;
		    }
		    break;
		}

		/*
		 * If we're in a comment now, skip to the start of the comment.
		 */					    /* XXX */
		if ((trypos = find_start_comment(ind_maxcomment)) != NULL)
		{
		    curwin->w_cursor.lnum = trypos->lnum + 1;
		    continue;
		}

		l = ml_get_curline();

		/*
		 * If this is a switch() label, may line up relative to that.
		 * if this is a C++ scope declaration, do the same.
		 */
		iscase = cin_iscase(l);
		if (iscase || cin_isscopedecl(l))
		{
		    /* When looking for a "do" we are not interested in
		     * labels. */
		    if (whilelevel > 0)
			continue;

		    /*
		     *	case xx:
		     *	    c = 99 +	    <- this indent plus continuation
		     *->	   here;
		     */
		    if (lookfor == LOOKFOR_UNTERM)
		    {
			amount += ind_continuation;
			break;
		    }

		    /*
		     *	case xx:	<- line up with this case
		     *	    x = 333;
		     *	case yy:
		     */
		    if (       (iscase && lookfor == LOOKFOR_CASE)
			    || (!iscase && lookfor == LOOKFOR_SCOPEDECL))
		    {
			/*
			 * Check that this case label is not for another
			 * switch()
			 */				    /* XXX */
			if ((trypos = find_start_brace(ind_maxcomment)) ==
					     NULL || trypos->lnum == ourscope)
			{
			    amount = get_indent();	/* XXX */
			    break;
			}
			continue;
		    }

		    n = get_indent_nolabel(curwin->w_cursor.lnum);  /* XXX */

		    /*
		     *	 case xx: if (cond)	    <- line up with this if
		     *		      y = y + 1;
		     * ->	  s = 99;
		     *
		     *	 case xx:
		     *	     if (cond)		<- line up with this line
		     *		 y = y + 1;
		     * ->    s = 99;
		     */
		    if (lookfor == LOOKFOR_TERM)
		    {
			if (n)
			    amount = n;
			break;
		    }

		    /*
		     *	 case xx: x = x + 1;	    <- line up with this x
		     * ->	  y = y + 1;
		     *
		     *	 case xx: if (cond)	    <- line up with this if
		     * ->	       y = y + 1;
		     */
		    if (n)
		    {
			amount = n;
			l = after_label(ml_get_curline());
			if (l != NULL && cin_is_cinword(l))
			    amount += ind_level + ind_no_brace;
			break;
		    }

		    /*
		     * Try to get the indent of a statement before the switch
		     * label.  If nothing is found, line up relative to the
		     * switch label.
		     *	    break;		<- may line up with this line
		     *	 case xx:
		     * ->   y = 1;
		     */
		    scope_amount = get_indent() + (iscase    /* XXX */
					? ind_case_code : ind_scopedecl_code);
		    lookfor = LOOKFOR_ANY;
		    continue;
		}

		/*
		 * Looking for a switch() label or C++ scope declaration,
		 * ignore other lines.
		 */
		if (lookfor == LOOKFOR_CASE || lookfor == LOOKFOR_SCOPEDECL)
		    continue;

		/*
		 * Ignore jump labels with nothing after them.
		 */
		if (cin_islabel(ind_maxcomment))
		{
		    l = after_label(ml_get_curline());
		    if (l == NULL || cin_nocode(l))
			continue;
		}

		/*
		 * Ignore #defines, #if, etc.
		 * Ignore comment and empty lines.
		 * (need to get the line again, cin_islabel() may have
		 * unlocked it)
		 */
		l = ml_get_curline();
		if (cin_ispreproc(l) || cin_nocode(l))
		    continue;

		/*
		 * What happens next depends on the line being terminated.
		 */
		if (!cin_isterminated(l, FALSE))
		{
		    /*
		     * if we're in the middle of a paren thing,
		     * go back to the line that starts it so
		     * we can get the right prevailing indent
		     *	   if ( foo &&
		     *		    bar )
		     */
		    /*
		     * position the cursor over the rightmost paren, so that
		     * matching it will take us back to the start of the line.
		     */
		    (void)find_last_paren(l);
		    if ((trypos = find_match_paren(ind_maxparen,
						     ind_maxcomment)) != NULL)
		    {
			/*
			 * Check if we are on a case label now.  This is
			 * handled above.
			 *     case xx:  if ( asdf &&
			 *			asdf)
			 */
			curwin->w_cursor.lnum = trypos->lnum;
			l = ml_get_curline();
			if (cin_iscase(l) || cin_isscopedecl(l))
			{
			    ++curwin->w_cursor.lnum;
			    continue;
			}
		    }

		    /*
		     * Get indent and pointer to text for current line,
		     * ignoring any jump label.	    XXX
		     */
		    cur_amount = skip_label(curwin->w_cursor.lnum,
							  &l, ind_maxcomment);

		    /*
		     * If this is just above the line we are indenting, and it
		     * starts with a '{', line it up with this line.
		     *		while (not)
		     * ->	{
		     *		}
		     */
		    if (lookfor != LOOKFOR_TERM && theline[0] == '{')
		    {
			amount = cur_amount;
			/*
			 * Only add ind_open_extra when the current line
			 * doesn't start with a '{', which must have a match
			 * in the same line (scope is the same).  Probably:
			 *	{ 1, 2 },
			 * ->	{ 3, 4 }
			 */
			if (*skipwhite(l) != '{')
			    amount += ind_open_extra;
			break;
		    }

		    /*
		     * Check if we are after an "if", "while", etc.
		     * Also allow "   } else".
		     */
		    if (cin_is_cinword(l) || cin_iselse(skipwhite(l)))
		    {
			/*
			 * Found an unterminated line after an if (), line up
			 * with the last one.
			 *   if (cond)
			 *	    100 +
			 * ->		here;
			 */
			if (lookfor == LOOKFOR_UNTERM)
			{
			    amount += ind_continuation;
			    break;
			}

			/*
			 * If this is just above the line we are indenting, we
			 * are finished.
			 *	    while (not)
			 * ->		here;
			 * Otherwise this indent can be used when the line
			 * before this is terminated.
			 *	yyy;
			 *	if (stat)
			 *	    while (not)
			 *		xxx;
			 * ->	here;
			 */
			amount = cur_amount;
			if (theline[0] == '{')
			    amount += ind_open_extra;
			if (lookfor != LOOKFOR_TERM)
			{
			    amount += ind_level + ind_no_brace;
			    break;
			}

			/*
			 * Special trick: when expecting the while () after a
			 * do, line up with the while()
			 *     do
			 *	    x = 1;
			 * ->  here
			 */
			l = skipwhite(ml_get_curline());
			if (cin_isdo(l))
			{
			    if (whilelevel == 0)
				break;
			    --whilelevel;
			}

			/*
			 * When searching for a terminated line, don't use the
			 * one between the "if" and the "else".
			 * Need to use the scope of this "else".  XXX
			 * If whilelevel != 0 continue looking for a "do {".
			 */
			if (cin_iselse(l)
				&& whilelevel == 0
				&& ((trypos = find_start_brace(ind_maxcomment))
								    == NULL
				    || find_match(LOOKFOR_IF, trypos->lnum,
					ind_maxparen, ind_maxcomment) == FAIL))
			    break;
		    }

		    /*
		     * If we're below an unterminated line that is not an
		     * "if" or something, we may line up with this line or
		     * add someting for a continuation line, depending on
		     * the line before this one.
		     */
		    else
		    {
			/*
			 * Found two unterminated lines on a row, line up with
			 * the last one.
			 *   c = 99 +
			 *	    100 +
			 * ->	    here;
			 */
			if (lookfor == LOOKFOR_UNTERM)
			    break;

			/*
			 * Found first unterminated line on a row, may line up
			 * with this line, remember its indent
			 *	    100 +
			 * ->	    here;
			 */
			amount = cur_amount;
			if (lookfor != LOOKFOR_TERM)
			    lookfor = LOOKFOR_UNTERM;
		    }
		}

		/*
		 * Check if we are after a while (cond);
		 * If so: Ignore until the matching "do".
		 */
							/* XXX */
		else if (cin_iswhileofdo(l,
					 curwin->w_cursor.lnum, ind_maxparen))
		{
		    /*
		     * Found an unterminated line after a while ();, line up
		     * with the last one.
		     *	    while (cond);
		     *	    100 +		<- line up with this one
		     * ->	    here;
		     */
		    if (lookfor == LOOKFOR_UNTERM)
		    {
			amount += ind_continuation;
			break;
		    }

		    if (whilelevel == 0)
		    {
			lookfor = LOOKFOR_TERM;
			amount = get_indent();	    /* XXX */
			if (theline[0] == '{')
			    amount += ind_open_extra;
		    }
		    ++whilelevel;
		}

		/*
		 * We are after a "normal" statement.
		 * If we had another statement we can stop now and use the
		 * indent of that other statement.
		 * Otherwise the indent of the current statement may be used,
		 * search backwards for the next "normal" statement.
		 */
		else
		{
		    /*
		     * Handle "do {" line.
		     */
		    if (whilelevel > 0)
		    {
			l = cin_skipcomment(ml_get_curline());
			if (cin_isdo(l))
			{
			    amount = get_indent();	/* XXX */
			    --whilelevel;
			    continue;
			}
		    }

		    /*
		     * Found a terminated line above an unterminated line. Add
		     * the amount for a continuation line.
		     *	 x = 1;
		     *	 y = foo +
		     * ->	here;
		     */
		    if (lookfor == LOOKFOR_UNTERM)
		    {
			amount += ind_continuation;
			break;
		    }

		    /*
		     * Found a terminated line above a terminated line or "if"
		     * etc. line. Use the amount of the line below us.
		     *	 x = 1;				x = 1;
		     *	 if (asdf)		    y = 2;
		     *	     while (asdf)	  ->here;
		     *		here;
		     * ->foo;
		     */
		    if (lookfor == LOOKFOR_TERM)
		    {
			if (whilelevel == 0)
			    break;
		    }

		    /*
		     * First line above the one we're indenting is terminated.
		     * To know what needs to be done look further backward for
		     * a terminated line.
		     */
		    else
		    {
			/*
			 * position the cursor over the rightmost paren, so
			 * that matching it will take us back to the start of
			 * the line.  Helps for:
			 *     func(asdr,
			 *	      asdfasdf);
			 *     here;
			 */
term_again:
			l = ml_get_curline();
			if (find_last_paren(l) &&
				(trypos = find_match_paren(ind_maxparen,
						     ind_maxcomment)) != NULL)
			{
			    /*
			     * Check if we are on a case label now.  This is
			     * handled above.
			     *	   case xx:  if ( asdf &&
			     *			    asdf)
			     */
			    curwin->w_cursor.lnum = trypos->lnum;
			    l = ml_get_curline();
			    if (cin_iscase(l) || cin_isscopedecl(l))
			    {
				++curwin->w_cursor.lnum;
				continue;
			    }
			}

			/*
			 * Get indent and pointer to text for current line,
			 * ignoring any jump label.
			 */
			amount = skip_label(curwin->w_cursor.lnum,
							  &l, ind_maxcomment);

			if (theline[0] == '{')
			    amount += ind_open_extra;
			/* See remark above: "Only add ind_open_extra.." */
			if (*skipwhite(l) == '{')
			    amount -= ind_open_extra;
			lookfor = LOOKFOR_TERM;

			/*
			 * If we're at the end of a block, skip to the start of
			 * that block.
			 */
			curwin->w_cursor.col = 0;
			if (*cin_skipcomment(l) == '}'
				&& (trypos = find_start_brace(ind_maxcomment))
							    != NULL) /* XXX */
			{
			    curwin->w_cursor.lnum = trypos->lnum;
			    /* if not "else {" check for terminated again */
			    /* but skip block for "} else {" */
			    l = cin_skipcomment(ml_get_curline());
			    if (*l == '}' || !cin_iselse(l))
				goto term_again;
			    ++curwin->w_cursor.lnum;
			}
		    }
		}
	    }
	}
      }
    }

    /*
     * ok -- we're not inside any sort of structure at all!
     *
     * this means we're at the top level, and everything should
     * basically just match where the previous line is, except
     * for the lines immediately following a function declaration,
     * which are K&R-style parameters and need to be indented.
     */
    else
    {
	/*
	 * if our line starts with an open brace, forget about any
	 * prevailing indent and make sure it looks like the start
	 * of a function
	 */

	if (theline[0] == '{')
	{
	    amount = ind_first_open;
	}

	/*
	 * If the NEXT line is a function declaration, the current
	 * line needs to be indented as a function type spec.
	 * Don't do this if the current line looks like a comment
	 * or if the current line is terminated, ie. ends in ';'.
	 */
	else if (cur_curpos.lnum < curbuf->b_ml.ml_line_count
		&& !cin_nocode(theline)
		&& cin_isfuncdecl(ml_get(cur_curpos.lnum + 1))
		&& !cin_isterminated(theline, FALSE))
	{
	    amount = ind_func_type;
	}
	else
	{
	    amount = 0;
	    curwin->w_cursor = cur_curpos;

	    /* search backwards until we find something we recognize */

	    while (curwin->w_cursor.lnum > 1)
	    {
		curwin->w_cursor.lnum--;
		curwin->w_cursor.col = 0;

		l = ml_get_curline();

		/*
		 * If we're in a comment now, skip to the start of the comment.
		 */						/* XXX */
		if ((trypos = find_start_comment(ind_maxcomment)) != NULL)
		{
		    curwin->w_cursor.lnum = trypos->lnum + 1;
		    continue;
		}

		/*
		 * If the line looks like a function declaration, and we're
		 * not in a comment, put it the left margin.
		 */
		if (cin_isfuncdecl(theline))
		    break;

		/*
		 * Finding the closing '}' of a previous function.  Put
		 * current line at the left margin.  For when 'cino' has "fs".
		 */
		if (*skipwhite(l) == '}')
		    break;

		/*
		 * If the previous line ends on '};' (maybe followed by
		 * comments) align at column 0.  For example:
		 * char *string_array[] = { "foo",
		 *     / * x * / "b};ar" }; / * foobar * /
		 */
		if (cin_ends_in(l, (char_u *)"};"))
		    break;

		/*
		 * Skip preprocessor directives and blank lines.
		 */
		if (cin_ispreproc(l))
		    continue;

		if (cin_nocode(l))
		    continue;

		/*
		 * If the PREVIOUS line is a function declaration, the current
		 * line (and the ones that follow) needs to be indented as
		 * parameters.
		 */
		if (cin_isfuncdecl(l))
		{
		    amount = ind_param;
		    break;
		}

		/*
		 * If the previous line ends in ';' and the line before the
		 * previous line ends in ',' or '\', ident to column zero:
		 * int foo,
		 *     bar;
		 * indent_to_0 here;
		 */
		if (cin_ends_in(l, (char_u*)";"))
		{
		    l = ml_get(curwin->w_cursor.lnum - 1);
		    if (cin_ends_in(l, (char_u *)",")
			    || (*l != NUL && l[STRLEN(l) - 1] == '\\'))
			break;
		    l = ml_get_curline();
		}

		/*
		 * If the previous line ends in ',', use one level of
		 * indentation:
		 * int foo,
		 *     bar;
		 */
		if (cin_ends_in(l, (char_u *)",")
			|| (*l != NUL && l[STRLEN(l) - 1] == '\\'))
		{
		    amount = get_indent();
		    if (amount == 0)
			amount = ind_param;
		    break;
		}

		/*
		 * Doesn't look like anything interesting -- so just
		 * use the indent of this line.
		 *
		 * Position the cursor over the rightmost paren, so that
		 * matching it will take us back to the start of the line.
		 */
		find_last_paren(l);

		if ((trypos = find_match_paren(ind_maxparen,
						     ind_maxcomment)) != NULL)
		    curwin->w_cursor.lnum = trypos->lnum;
		amount = get_indent();	    /* XXX */
		break;
	    }
	}
    }

theend:
    /* put the cursor back where it belongs */
    curwin->w_cursor = cur_curpos;

    vim_free(linecopy);

    if (amount < 0)
	return 0;
    return amount;
}

    static int
find_match(lookfor, ourscope, ind_maxparen, ind_maxcomment)
    int		lookfor;
    linenr_T	ourscope;
    int		ind_maxparen;
    int		ind_maxcomment;
{
    char_u	*look;
    pos_T	*theirscope;
    char_u	*mightbeif;
    int		elselevel;
    int		whilelevel;

    if (lookfor == LOOKFOR_IF)
    {
	elselevel = 1;
	whilelevel = 0;
    }
    else
    {
	elselevel = 0;
	whilelevel = 1;
    }

    curwin->w_cursor.col = 0;

    while (curwin->w_cursor.lnum > ourscope + 1)
    {
	curwin->w_cursor.lnum--;
	curwin->w_cursor.col = 0;

	look = cin_skipcomment(ml_get_curline());
	if (cin_iselse(look)
		|| cin_isif(look)
		|| cin_isdo(look)			    /* XXX */
		|| cin_iswhileofdo(look, curwin->w_cursor.lnum, ind_maxparen))
	{
	    /*
	     * if we've gone outside the braces entirely,
	     * we must be out of scope...
	     */
	    theirscope = find_start_brace(ind_maxcomment);  /* XXX */
	    if (theirscope == NULL)
		break;

	    /*
	     * and if the brace enclosing this is further
	     * back than the one enclosing the else, we're
	     * out of luck too.
	     */
	    if (theirscope->lnum < ourscope)
		break;

	    /*
	     * and if they're enclosed in a *deeper* brace,
	     * then we can ignore it because it's in a
	     * different scope...
	     */
	    if (theirscope->lnum > ourscope)
		continue;

	    /*
	     * if it was an "else" (that's not an "else if")
	     * then we need to go back to another if, so
	     * increment elselevel
	     */
	    look = cin_skipcomment(ml_get_curline());
	    if (cin_iselse(look))
	    {
		mightbeif = cin_skipcomment(look + 4);
		if (!cin_isif(mightbeif))
		    ++elselevel;
		continue;
	    }

	    /*
	     * if it was a "while" then we need to go back to
	     * another "do", so increment whilelevel.  XXX
	     */
	    if (cin_iswhileofdo(look, curwin->w_cursor.lnum, ind_maxparen))
	    {
		++whilelevel;
		continue;
	    }

	    /* If it's an "if" decrement elselevel */
	    look = cin_skipcomment(ml_get_curline());
	    if (cin_isif(look))
	    {
		elselevel--;
		/*
		 * When looking for an "if" ignore "while"s that
		 * get in the way.
		 */
		if (elselevel == 0 && lookfor == LOOKFOR_IF)
		    whilelevel = 0;
	    }

	    /* If it's a "do" decrement whilelevel */
	    if (cin_isdo(look))
		whilelevel--;

	    /*
	     * if we've used up all the elses, then
	     * this must be the if that we want!
	     * match the indent level of that if.
	     */
	    if (elselevel <= 0 && whilelevel <= 0)
	    {
		return OK;
	    }
	}
    }
    return FAIL;
}

# if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Get indent level from 'indentexpr'.
 */
    int
get_expr_indent()
{
    int		indent;
    pos_T	pos = curwin->w_cursor;

    set_vim_var_nr(VV_LNUM, curwin->w_cursor.lnum);
    ++sandbox;
    indent = eval_to_number(curbuf->b_p_inde);
    --sandbox;

    /* restore the cursor position so that 'indentexpr' doesn't need to */
    curwin->w_cursor = pos;
    check_cursor();

    /* If there is an error, just keep the current indent. */
    if (indent < 0)
	indent = get_indent();

    return indent;
}
# endif

#endif /* FEAT_CINDENT */

#ifdef FEAT_LISP

static int lisp_match __ARGS((char_u *p));

    static int
lisp_match(p)
    char_u	*p;
{
    char_u	buf[LSIZE];
    int		len;
    char_u	*word = p_lispwords;

    while (*word != NUL)
    {
	(void)copy_option_part(&word, buf, LSIZE, ",");
	len = (int)STRLEN(buf);
	if (STRNCMP(buf, p, len) == 0 && p[len] == ' ')
	    return TRUE;
    }
    return FALSE;
}

/*
 * When 'p' is present in 'cpoptions, a Vi compatible method is used.
 * The incompatible newer method is quite a bit better at indenting
 * code in lisp-like languages than the traditional one; it's still
 * mostly heuristics however -- Dirk van Deun, dirk@rave.org
 *
 * TODO:
 * Findmatch() should be adapted for lisp, also to make showmatch
 * work correctly: now (v5.3) it seems all C/C++ oriented:
 * - it does not recognize the #\( and #\) notations as character literals
 * - it doesn't know about comments starting with a semicolon
 * - it incorrectly interprets '(' as a character literal
 * All this messes up get_lisp_indent in some rare cases.
 */
    int
get_lisp_indent()
{
    pos_T	*pos, realpos;
    int		amount;
    char_u	*that;
    colnr_T	col;
    colnr_T	firsttry;
    int		parencount, quotecount;
    int		vi_lisp;

    /* Set vi_lisp to use the vi-compatible method */
    vi_lisp = (vim_strchr(p_cpo, CPO_LISP) != NULL);

    realpos = curwin->w_cursor;
    curwin->w_cursor.col = 0;

    if ((pos = findmatch(NULL, '(')) != NULL)
    {
	/* Extra trick: Take the indent of the first previous non-white
	 * line that is at the same () level. */
	amount = -1;
	parencount = 0;

	while (--curwin->w_cursor.lnum >= pos->lnum)
	{
	    if (linewhite(curwin->w_cursor.lnum))
		continue;
	    for (that = ml_get_curline(); *that != NUL; ++that)
	    {
                if (*that == ';')
                {
                    while (*(that + 1) != NUL)
			++that;
                    continue;
                }
		if (*that == '\\')
		{
		    if (*(that + 1) != NUL)
			++that;
		    continue;
		}
		if (*that == '"' && *(that + 1) != NUL)
		{
		    that++;
		    while (*that && (*that != '"' || *(that - 1) == '\\'))
			++that;
		}
		if (*that == '(')
                    ++parencount;
		else if (*that == ')')
                    --parencount;
	    }
	    if (parencount == 0)
	    {
		amount = get_indent();
		break;
	    }
	}

	if (amount == -1)
	{
	    curwin->w_cursor.lnum = pos->lnum;
	    curwin->w_cursor.col = pos->col;
	    col = pos->col;

	    that = ml_get_curline();

	    if (vi_lisp && get_indent() == 0)
		amount = 2;
	    else
	    {
		amount = 0;
		while (*that && col)
		{
		    amount += lbr_chartabsize_adv(&that, (colnr_T)amount);
		    col--;
		}

		/*
		 * Some keywords require "body" indenting rules (the
		 * non-standard-lisp ones are Scheme special forms):
		 *
		 * (let ((a 1))    instead    (let ((a 1))
		 *   (...))           of           (...))
		 */

		if (!vi_lisp && *that == '(' && lisp_match(that + 1))
		    amount += 2;
		else
		{
		    that++;
		    amount++;
		    firsttry = amount;

		    while (vim_iswhite(*that))
		    {
			amount += lbr_chartabsize(that, (colnr_T)amount);
			++that;
		    }

		    if (*that && *that != ';') /* not a comment line */
		    {
                        /* test *that != '(' to accomodate first let/do
                         * argument if it is more than one line */
			if (!vi_lisp && *that != '(')
			    firsttry++;

                        parencount = 0;
			quotecount = 0;

			if (vi_lisp
				|| (*that != '"'
				    && *that != '\''
				    && *that != '#'
				    && (*that < '0' || *that > '9')))
			{
			    while (*that
				    && (!vim_iswhite(*that)
					|| quotecount
					|| parencount)
				    && (!(*that == '('
					    && !quotecount
					    && !parencount
					    && vi_lisp)))
                            {
                                if (*that == '"')
				    quotecount = !quotecount;
                                if (*that == '(' && !quotecount)
                                    ++parencount;
                                if (*that == ')' && !quotecount)
                                    --parencount;
                                if (*that == '\\' && *(that+1) != NUL)
                                    amount += lbr_chartabsize_adv(&that,
							     (colnr_T)amount);
                                amount += lbr_chartabsize_adv(&that,
							     (colnr_T)amount);
                            }
			}
                        while (vim_iswhite(*that))
                        {
                            amount += lbr_chartabsize(that, (colnr_T)amount);
                            that++;
                        }
                        if (!*that || *that == ';')
                            amount = firsttry;
                    }
                }
            }
        }
    }
    else
        amount = 0;	/* no matching '(' found, use zero indent */

    curwin->w_cursor = realpos;

    return amount;
}
#endif /* FEAT_LISP */

    void
prepare_to_exit()
{
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	gui.dying = TRUE;
	out_trash();	/* trash any pending output */
    }
    else
#endif
    {
	windgoto((int)Rows - 1, 0);

	/*
	 * Switch terminal mode back now, so messages end up on the "normal"
	 * screen (if there are two screens).
	 */
	settmode(TMODE_COOK);
#ifdef WIN3264
	if (can_end_termcap_mode(FALSE) == TRUE)
#endif
	    stoptermcap();
	out_flush();
    }
}

/*
 * Preserve files and exit.
 * When called IObuff must contain a message.
 */
    void
preserve_exit()
{
    buf_T	*buf;

    prepare_to_exit();

    out_str(IObuff);
    screen_start();		    /* don't know where cursor is now */
    out_flush();

    ml_close_notmod();		    /* close all not-modified buffers */

    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
    {
	if (buf->b_ml.ml_mfp != NULL && buf->b_ml.ml_mfp->mf_fname != NULL)
	{
	    OUT_STR(_("Vim: preserving files...\n"));
	    screen_start();	    /* don't know where cursor is now */
	    out_flush();
	    ml_sync_all(FALSE, FALSE);	/* preserve all swap files */
	    break;
	}
    }

    ml_close_all(FALSE);	    /* close all memfiles, without deleting */

    OUT_STR(_("Vim: Finished.\n"));

    getout(1);
}

/*
 * return TRUE if "fname" exists.
 */
    int
vim_fexists(fname)
    char_u  *fname;
{
    struct stat st;

    if (mch_stat((char *)fname, &st))
	return FALSE;
    return TRUE;
}

/*
 * Check for CTRL-C pressed, but only once in a while.
 * Should be used instead of ui_breakcheck() for functions that check for
 * each line in the file.  Calling ui_breakcheck() each time takes too much
 * time, because it can be a system call.
 */

#ifndef BREAKCHECK_SKIP
# ifdef FEAT_GUI		    /* assume the GUI only runs on fast computers */
#  define BREAKCHECK_SKIP 200
# else
#  define BREAKCHECK_SKIP 32
# endif
#endif

static int	breakcheck_count = 0;

    void
line_breakcheck()
{
    if (++breakcheck_count >= BREAKCHECK_SKIP)
    {
	breakcheck_count = 0;
	ui_breakcheck();
    }
}

/*
 * Like line_breakcheck() but check 10 times less often.
 */
    void
fast_breakcheck()
{
    if (++breakcheck_count >= BREAKCHECK_SKIP * 10)
    {
	breakcheck_count = 0;
	ui_breakcheck();
    }
}

/*
 * Expand wildcards.  Calls gen_expand_wildcards() and removes files matching
 * 'wildignore'.
 */
    int
expand_wildcards(num_pat, pat, num_file, file, flags)
    int		   num_pat;	/* number of input patterns */
    char_u	 **pat;		/* array of input patterns */
    int		  *num_file;	/* resulting number of files */
    char_u	***file;	/* array of resulting files */
    int		   flags;	/* EW_DIR, etc. */
{
    int		retval;
    int		i, j;
    char_u	*p;
    int		non_suf_match;	/* number without matching suffix */

    retval = gen_expand_wildcards(num_pat, pat, num_file, file, flags);

    /* When keeping all matches, return here */
    if (flags & EW_KEEPALL)
	return retval;

#ifdef FEAT_WILDIGN
    /*
     * Remove names that match 'wildignore'.
     */
    if (*p_wig)
    {
	char_u	*ffname;

	/* check all files in (*file)[] */
	for (i = 0; i < *num_file; ++i)
	{
	    ffname = FullName_save((*file)[i], FALSE);
	    if (ffname == NULL)		/* out of memory */
		break;
#ifdef VMS
	    vms_remove_version(ffname);
#endif
	    if (match_file_list(p_wig, (*file)[i], ffname))
	    {
		/* remove this matching file from the list */
		vim_free((*file)[i]);
		for (j = i; j + 1 < *num_file; ++j)
		    (*file)[j] = (*file)[j + 1];
		--*num_file;
		--i;
	    }
	    vim_free(ffname);
	}
    }
#endif

    /*
     * Move the names where 'suffixes' match to the end.
     */
    if (*num_file > 1)
    {
	non_suf_match = 0;
	for (i = 0; i < *num_file; ++i)
	{
	    if (!match_suffix((*file)[i]))
	    {
		/*
		 * Move the name without matching suffix to the front
		 * of the list.
		 */
		p = (*file)[i];
		for (j = i; j > non_suf_match; --j)
		    (*file)[j] = (*file)[j - 1];
		(*file)[non_suf_match++] = p;
	    }
	}
    }

    return retval;
}

/*
 * Return TRUE if "fname" matches with an entry in 'suffixes'.
 */
    int
match_suffix(fname)
    char_u	*fname;
{
    int		fnamelen, setsuflen;
    char_u	*setsuf;
#define MAXSUFLEN 30	    /* maximum length of a file suffix */
    char_u	suf_buf[MAXSUFLEN];

    fnamelen = (int)STRLEN(fname);
    setsuflen = 0;
    for (setsuf = p_su; *setsuf; )
    {
	setsuflen = copy_option_part(&setsuf, suf_buf, MAXSUFLEN, ".,");
	if (fnamelen >= setsuflen
		&& fnamencmp(suf_buf, fname + fnamelen - setsuflen,
					      (size_t)setsuflen) == 0)
	    break;
	setsuflen = 0;
    }
    return (setsuflen != 0);
}

#if !defined(NO_EXPANDPATH) || defined(PROTO)

# ifdef VIM_BACKTICK
static int vim_backtick __ARGS((char_u *p));
static int expand_backtick __ARGS((garray_T *gap, char_u *pat, int flags));
# endif

# if defined(MSDOS) || defined(FEAT_GUI_W16) || defined(WIN3264)
/*
 * File name expansion code for MS-DOS, Win16 and Win32.  It's here because
 * it's shared between these systems.
 */
#ifdef WIN3264
# include <windows.h>
#endif

# if defined(DJGPP) || defined(PROTO)
#  define _cdecl	    /* DJGPP doesn't have this */
# else
#  ifdef __BORLANDC__
#   define _cdecl _RTLENTRYF
#  endif
# endif

/*
 * comparison function for qsort in dos_expandpath()
 */
    static int _cdecl
pstrcmp(const void *a, const void *b)
{
    return (pathcmp(*(char **)a, *(char **)b));
}

# ifndef WIN3264
    static void
namelowcpy(
    char_u *d,
    char_u *s)
{
#  ifdef DJGPP
    if (USE_LONG_FNAME)	    /* don't lower case on Windows 95/NT systems */
	while (*s)
	    *d++ = *s++;
    else
#  endif
	while (*s)
	    *d++ = TO_LOWER(*s++);
    *d = NUL;
}
# endif

/*
 * Recursively build up a list of files in "gap" matching the first wildcard
 * in `path'.  Called by expand_wildcards().
 * Return the number of matches found.
 * "path" has backslashes before chars that are not to be expanded, starting
 * at "wildc".
 */
    static int
dos_expandpath(
    garray_T	*gap,
    char_u	*path,
    char_u	*wildc,
    int		flags)		/* EW_* flags */
{
    char_u		*buf;
    char_u		*p, *s, *e;
    int			start_len = gap->ga_len;
    int			ok;
#ifdef WIN3264
    WIN32_FIND_DATA	fb;
    HANDLE		hFind;
#else
    struct ffblk	fb;
#endif
    int			matches;
    int			starts_with_dot;
    int			len;
    char_u		*pat;
    regmatch_T		regmatch;
    char_u		*matchname;

    /* make room for file name */
    buf = alloc((unsigned int)STRLEN(path) + BASENAMELEN + 5);
    if (buf == NULL)
	return 0;

    /*
     * Find the first part in the path name that contains a wildcard.
     * Copy it into buf, including the preceding characters.
     */
    p = buf;
    s = buf;
    e = NULL;
    while (*path != NUL)
    {
	if (path >= wildc && rem_backslash(path))	/* remove a backslash */
	    ++path;
	else if (*path == '\\' || *path == ':' || *path == '/')
	{
	    if (e != NULL)
		break;
	    else
		s = p + 1;
	}
	else if (*path == '*' || *path == '?' || *path == '[')
	    e = p;
#ifdef FEAT_MBYTE
	if (has_mbyte)
	{
	    len = (*mb_ptr2len_check)(path);
	    STRNCPY(p, path, len);
	    p += len;
	    path += len;
	}
	else
#endif
	    *p++ = *path++;
    }
    e = p;
    *e = NUL;
    /* now we have one wildcard component between s and e */

    starts_with_dot = (*s == '.');
    pat = file_pat_to_reg_pat(s, e, NULL, FALSE);
    if (pat == NULL)
    {
	vim_free(buf);
	return 0;
    }

    /* compile the regexp into a program */
    regmatch.rm_ic = TRUE;		/* Always ignore case */
    regmatch.regprog = vim_regcomp(pat, TRUE);
    vim_free(pat);

    if (regmatch.regprog == NULL)
    {
	vim_free(buf);
	return 0;
    }

    /* remember the pattern or file name being looked for */
    matchname = vim_strsave(s);

    /* Scan all files in the directory with "dir/ *.*" */
    STRCPY(s, "*.*");
#ifdef WIN3264
    hFind = FindFirstFile(buf, &fb);
    ok = (hFind != INVALID_HANDLE_VALUE);
#else
    /* If we are expanding wildcards we try both files and directories */
    ok = (findfirst((char *)buf, &fb,
			    (*path || (flags & EW_DIR)) ? FA_DIREC : 0) == 0);
#endif

    while (ok)
    {
#ifdef WIN3264
	p = (char_u *)fb.cFileName;
#else
	p = (char_u *)fb.ff_name;
#endif
	/* Ignore entries starting with a dot, unless when asked for.  Accept
	 * all entries found with "matchname". */
	if ((p[0] != '.' || starts_with_dot)
		&& (matchname == NULL
		    || vim_regexec(&regmatch, p, (colnr_T)0)))
	{
#ifdef WIN3264
	    STRCPY(s, p);
#else
	    namelowcpy(s, p);
#endif
	    len = (int)STRLEN(buf);
	    STRCPY(buf + len, path);
	    if (mch_has_wildcard(path))
	    {
		/* need to expand another component of the path */
		/* remove backslashes for the remaining components only */
		(void)dos_expandpath(gap, buf, buf + len + 1, flags);
	    }
	    else
	    {
		/* no more wildcards, check if there is a match */
		/* remove backslashes for the remaining components only */
		if (*path)
		    backslash_halve(buf + len + 1);
		if (mch_getperm(buf) >= 0)	/* add existing file */
		    addfile(gap, buf, flags);
	    }
	}
#ifdef WIN3264
	ok = FindNextFile(hFind, &fb);
#else
	ok = (findnext(&fb) == 0);
#endif

	/* If no more matches and no match was used, try expanding the name
	 * itself.  Finds the long name of a short filename. */
	if (!ok && matchname != NULL && gap->ga_len == start_len)
	{
	    STRCPY(s, matchname);
#ifdef WIN3264
	    FindClose(hFind);
	    hFind = FindFirstFile(buf, &fb);
	    ok = (hFind != INVALID_HANDLE_VALUE);
#else
	    ok = (findfirst((char *)buf, &fb,
			    (*path || (flags & EW_DIR)) ? FA_DIREC : 0) == 0);
#endif
	    vim_free(matchname);
	    matchname = NULL;
	}
    }

#ifdef WIN3264
    FindClose(hFind);
#endif
    vim_free(buf);
    vim_free(regmatch.regprog);
    vim_free(matchname);

    matches = gap->ga_len - start_len;
    if (matches)
	qsort(((char_u **)gap->ga_data) + start_len, (size_t)matches,
						   sizeof(char_u *), pstrcmp);
    return matches;
}

    int
mch_expandpath(
    garray_T	*gap,
    char_u	*path,
    int		flags)		/* EW_* flags */
{
    return dos_expandpath(gap, path, path, flags);
}
# endif /* MSDOS || FEAT_GUI_W16 */

/*
 * Generic wildcard expansion code.
 *
 * Characters in "pat" that should not be expanded must be preceded with a
 * backslash. E.g., "/path\ with\ spaces/my\*star*"
 *
 * Return FAIL when no single file was found.  In this case "num_file" is not
 * set, and "file" may contain an error message.
 * Return OK when some files found.  "num_file" is set to the number of
 * matches, "file" to the array of matches.  Call FreeWild() later.
 */
    int
gen_expand_wildcards(num_pat, pat, num_file, file, flags)
    int		num_pat;	/* number of input patterns */
    char_u	**pat;		/* array of input patterns */
    int		*num_file;	/* resulting number of files */
    char_u	***file;	/* array of resulting files */
    int		flags;		/* EW_* flags */
{
    int			i;
    garray_T		ga;
    char_u		*p;
    static int		recursive = FALSE;
    int			add_pat;

    /*
     * expand_env() is called to expand things like "~user".  If this fails,
     * it calls ExpandOne(), which brings us back here.  In this case, always
     * call the machine specific expansion function, if possible.  Otherwise,
     * return FAIL.
     */
    if (recursive)
#ifdef SPECIAL_WILDCHAR
	return mch_expand_wildcards(num_pat, pat, num_file, file, flags);
#else
	return FAIL;
#endif

#ifdef SPECIAL_WILDCHAR
    /*
     * If there are any special wildcard characters which we cannot handle
     * here, call machine specific function for all the expansion.  This
     * avoids starting the shell for each argument separately.
     * For `=expr` do use the internal function.
     */
    for (i = 0; i < num_pat; i++)
    {
	if (vim_strpbrk(pat[i], (char_u *)SPECIAL_WILDCHAR) != NULL
# ifdef VIM_BACKTICK
		&& !(vim_backtick(pat[i]) && pat[i][1] == '=')
# endif
	   )
	    return mch_expand_wildcards(num_pat, pat, num_file, file, flags);
    }
#endif

    recursive = TRUE;

    /*
     * The matching file names are stored in a growarray.  Init it empty.
     */
    ga_init2(&ga, (int)sizeof(char_u *), 30);

    for (i = 0; i < num_pat; ++i)
    {
	add_pat = -1;
	p = pat[i];

#ifdef VIM_BACKTICK
	if (vim_backtick(p))
	    add_pat = expand_backtick(&ga, p, flags);
	else
#endif
	{
	    /*
	     * First expand environment variables, "~/" and "~user/".
	     */
	    if (vim_strpbrk(p, (char_u *)"$~") != NULL)
	    {
		p = expand_env_save(p);
		if (p == NULL)
		    p = pat[i];
#ifdef UNIX
		/*
		 * On Unix, if expand_env() can't expand an environment
		 * variable, use the shell to do that.  Discard previously
		 * found file names and start all over again.
		 */
		else if (vim_strpbrk(p, (char_u *)"$~") != NULL)
		{
		    vim_free(p);
		    ga_clear(&ga);
		    i = mch_expand_wildcards(num_pat, pat, num_file, file,
								       flags);
		    recursive = FALSE;
		    return i;
		}
#endif
	    }

	    /*
	     * If there are wildcards: Expand file names and add each match to
	     * the list.  If there is no match, and EW_NOTFOUND is given, add
	     * the pattern.
	     * If there are no wildcards: Add the file name if it exists or
	     * when EW_NOTFOUND is given.
	     */
	    if (mch_has_wildcard(p))
		add_pat = mch_expandpath(&ga, p, flags);
	}

	if (add_pat == -1 || (add_pat == 0 && (flags & EW_NOTFOUND)))
	{
	    char_u	*t = backslash_halve_save(p);

#if defined(MACOS_CLASSIC)
	    slash_to_colon(t);
#endif
	    /* When EW_NOTFOUND is used, always add files and dirs.  Makes
	     * "vim c:/" work. */
	    if (flags & EW_NOTFOUND)
		addfile(&ga, t, flags | EW_DIR | EW_FILE);
	    else if (mch_getperm(t) >= 0)
		addfile(&ga, t, flags);
	    vim_free(t);
	}

	if (p != pat[i])
	    vim_free(p);
    }

    *num_file = ga.ga_len;
    *file = (ga.ga_data != NULL) ? (char_u **)ga.ga_data : (char_u **)"";

    recursive = FALSE;

    return (ga.ga_data != NULL) ? OK : FAIL;
}

# ifdef VIM_BACKTICK

/*
 * Return TRUE if we can expand this backtick thing here.
 */
    static int
vim_backtick(p)
    char_u	*p;
{
    return (*p == '`' && *(p + 1) != NUL && *(p + STRLEN(p) - 1) == '`');
}

/*
 * Expand an item in `backticks` by executing it as a command.
 * Currently only works when pat[] starts and ends with a `.
 * Returns number of file names found.
 */
    static int
expand_backtick(gap, pat, flags)
    garray_T	*gap;
    char_u	*pat;
    int		flags;	/* EW_* flags */
{
    char_u	*p;
    char_u	*cmd;
    char_u	*buffer;
    int		cnt = 0;
    int		i;

    /* Create the command: lop off the backticks. */
    cmd = vim_strnsave(pat + 1, (int)STRLEN(pat) - 2);
    if (cmd == NULL)
	return 0;

#ifdef FEAT_EVAL
    if (*cmd == '=')	    /* `={expr}`: Expand expression */
	buffer = eval_to_string(cmd + 1, &p);
    else
#endif
	buffer = get_cmd_output(cmd, (flags & EW_SILENT) ? SHELL_SILENT : 0);
    vim_free(cmd);
    if (buffer == NULL)
	return 0;

    cmd = buffer;
    while (*cmd != NUL)
    {
	cmd = skipwhite(cmd);		/* skip over white space */
	p = cmd;
	while (*p != NUL && *p != '\r' && *p != '\n') /* skip over entry */
	    ++p;
	/* add an entry if it is not empty */
	if (p > cmd)
	{
	    i = *p;
	    *p = NUL;
	    addfile(gap, cmd, flags);
	    *p = i;
	    ++cnt;
	}
	cmd = p;
	while (*cmd != NUL && (*cmd == '\r' || *cmd == '\n'))
	    ++cmd;
    }

    vim_free(buffer);
    return cnt;
}
# endif /* VIM_BACKTICK */

/*
 * Add a file to a file list.  Accepted flags:
 * EW_DIR	add directories
 * EW_FILE	add files
 * EW_NOTFOUND	add even when it doesn't exist
 * EW_ADDSLASH	add slash after directory name
 */
    void
addfile(gap, f, flags)
    garray_T	*gap;
    char_u	*f;	/* filename */
    int		flags;
{
    char_u	*p;
    int		isdir;

    /* if the file/dir doesn't exist, may not add it */
    if (!(flags & EW_NOTFOUND) && mch_getperm(f) < 0)
	return;

#ifdef FNAME_ILLEGAL
    /* if the file/dir contains illegal characters, don't add it */
    if (vim_strpbrk(f, (char_u *)FNAME_ILLEGAL) != NULL)
	return;
#endif

    isdir = mch_isdir(f);
    if ((isdir && !(flags & EW_DIR)) || (!isdir && !(flags & EW_FILE)))
	return;

    /* Make room for another item in the file list. */
    if (ga_grow(gap, 1) == FAIL)
	return;

    p = alloc((unsigned)(STRLEN(f) + 1 + isdir));
    if (p == NULL)
	return;

    STRCPY(p, f);
#ifdef BACKSLASH_IN_FILENAME
    slash_adjust(p);
#endif
    /*
     * Append a slash or backslash after directory names if none is present.
     */
#ifndef DONT_ADD_PATHSEP_TO_DIR
    if (isdir && (flags & EW_ADDSLASH))
	add_pathsep(p);
#endif
    ((char_u **)gap->ga_data)[gap->ga_len++] = p;
    --gap->ga_room;
}
#endif /* !NO_EXPANDPATH */

#if defined(VIM_BACKTICK) || defined(FEAT_EVAL) || defined(PROTO)

#ifndef SEEK_SET
# define SEEK_SET 0
#endif
#ifndef SEEK_END
# define SEEK_END 2
#endif

/*
 * Get the stdout of an external command.
 * Returns an allocated string, or NULL for error.
 */
    char_u *
get_cmd_output(cmd, flags)
    char_u	*cmd;
    int		flags;		/* can be SHELL_SILENT */
{
    char_u	*tempname;
    char_u	*command;
    char_u	*buffer = NULL;
    int		len;
    int		i = 0;
    FILE	*fd;

    if (check_restricted() || check_secure())
	return NULL;

    /* get a name for the temp file */
    if ((tempname = vim_tempname('o')) == NULL)
    {
	EMSG(_(e_notmp));
	return NULL;
    }

    /* Add the redirection stuff */
    command = make_filter_cmd(cmd, NULL, tempname);
    if (command == NULL)
	goto done;

    /*
     * Call the shell to execute the command (errors are ignored).
     */
    call_shell(command, SHELL_DOOUT | SHELL_EXPAND | flags);
    vim_free(command);

    /*
     * read the names from the file into memory
     */
    fd = mch_fopen((char *)tempname, "r");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), tempname);
	goto done;
    }

    fseek(fd, 0L, SEEK_END);
    len = ftell(fd);		    /* get size of temp file */
    fseek(fd, 0L, SEEK_SET);

    buffer = alloc(len + 1);
    if (buffer != NULL)
	i = (int)fread((char *)buffer, (size_t)1, (size_t)len, fd);
    fclose(fd);
    mch_remove(tempname);
    if (buffer == NULL)
	goto done;
    if (i != len)
    {
	EMSG2(_(e_notread), tempname);
	vim_free(buffer);
	buffer = NULL;
    }
    else
	buffer[len] = '\0';	/* make sure the buffer is terminated */

done:
    vim_free(tempname);
    return buffer;
}
#endif

/*
 * Free the list of files returned by expand_wildcards() or other expansion
 * functions.
 */
    void
FreeWild(count, files)
    int	    count;
    char_u  **files;
{
    if (files == NULL || count <= 0)
	return;
#if defined(__EMX__) && defined(__ALWAYS_HAS_TRAILING_NULL_POINTER) /* XXX */
    /*
     * Is this still OK for when other functions than expand_wildcards() have
     * been used???
     */
    _fnexplodefree((char **)files);
#else
    while (count--)
	vim_free(files[count]);
    vim_free(files);
#endif
}

/*
 * return TRUE when need to go to Insert mode because of 'insertmode'.
 * Don't do this when still processing a command or a mapping.
 * Don't do this when inside a ":normal" command.
 */
    int
goto_im()
{
    return (p_im && stuff_empty() && typebuf_typed());
}
