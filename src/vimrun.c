/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *			this file by Vince Negri
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * vimrun.c - Tiny Win32 program to safely run an external command in a
 *	      DOS console.
 */

#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

#ifdef __BORLANDC__
extern char *
#ifdef _RTLDLL
__import
#endif
_oscmd;
# define _kbhit kbhit
# define _getch getch
#else
extern char *_acmdln;
#endif

    int
main(void)
{
    const char	*p;
    int		retval;

#ifdef __BORLANDC__
    p = _oscmd;
#else
    p = _acmdln;
#endif
    /*
     * Skip the executable name
     */
    while (*p++ != ' ')
	;

    /* Print the command, including quotes and redirection. */
    puts(p);
    retval = system(p);
    if (retval != 0)
	printf("%d returned\n", retval);

    puts("Hit any key to close this window...");

    while (_kbhit())
	(void)_getch();
    (void)_getch();

    return retval;
}
