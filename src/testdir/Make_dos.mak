#
# Makefile to run al tests for Vim, on Dos-like machines.
#
# Requires a set of Unix tools: echo, diff, etc.

VIMPROG = ..\\vim

# Omitted:
# test2		"\\tmp" doesn't work.
# test10	'errorformat' is different
# test12	can't unlink a swap file
# test25	uses symbolic link
# test27	can't edit file with "*" in file name
# test31	16 bit version runs out of memory...

SCRIPTS16 =	test1.out test19.out test20.out test22.out \
		test23.out test24.out test28.out test29.out \
		test35.out test36.out test43.out \
		test44.out

SCRIPTS =	test3.out test4.out test5.out test6.out test7.out \
		test8.out test9.out test11.out test13.out test14.out \
		test15.out test17.out test18.out test21.out test26.out \
		test30.out test31.out test32.out test33.out test34.out \
		test37.out test38.out test39.out test40.out test41.out \
		test42.out

SCRIPTS_GUI = test16.out

.SUFFIXES: .in .out

nongui:	/tmp $(SCRIPTS16) $(SCRIPTS)
	echo ALL DONE

small:	/tmp $(SCRIPTS16)
	echo ALL DONE

gui:	/tmp $(SCRIPTS16) $(SCRIPTS) $(SCRIPTS_GUI)
	echo ALL DONE

clean:
	-del *.out
	-del test.ok
	-del small.vim
	-del tiny.vim
	-del mbyte.vim
	-deltree /y X*

.in.out:
	copy $*.ok test.ok
	$(VIMPROG) -u dos.vim -U NONE --noplugin -s dotest.in $*.in
	diff test.out $*.ok
	rename test.out $*.out
	-deltree /y X*
	-del test.ok


# Create a directory for temp files
/tmp:
	-mkdir /tmp