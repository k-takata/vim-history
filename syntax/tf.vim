" Vim syntax file
" Language:	tf
" Maintainer:	Lutz Eymers <ixtab@polzin.com>
" URL:		http://www-public.rz.uni-duesseldorf.de/~eymers/stuff/syntax_vim.tgz
" Email:        send syntax_vim.tgz
" Last change:	1998 May 8

" Remove any old syntax stuff hanging around
syn clear

syn case match

" Special global variables
syn keyword tfVar  HOME LANG MAIL SHELL TERM TFHELP TFLIBDIR TFLIBRARY TZ  contained
syn keyword tfVar  background backslash  contained
syn keyword tfVar  bamf bg_output borg clearfull cleardone clock connect  contained
syn keyword tfVar  emulation end_color gag gethostbyname gpri hook hilite  contained
syn keyword tfVar  hiliteattr histsize hpri insert isize istrip kecho  contained
syn keyword tfVar  kprefix login lp lpquote maildelay matching max_iter  contained
syn keyword tfVar  max_recur mecho more mprefix oldslash promt_sec  contained
syn keyword tfVar  prompt_usec proxy_host proxy_port ptime qecho qprefix  contained
syn keyword tfVar  quite quitdone redef refreshtime scroll shpause snarf sockmload  contained
syn keyword tfVar  start_color tabsize telopt sub time_format visual  contained
syn keyword tfVar  watch_dog watchname wordpunct wrap wraplog wrapsize  contained
syn keyword tfVar  wrapspace  contained

" Worldvar
syn keyword tfWorld  world_name world_character world_password world_host contained
syn keyword tfWorld  world_port world_mfile world_type contained

" Readonly Var
syn match tfReadonly  "[#*]" contained
syn match tfReadonly  "\<-\=L\=\d\+\>" contained
syn match tfReadonly  "\<P\d\+\>" contained
syn match tfReadonly  "\<R\>" contained

" Identifier
syn match  tfIdentifier1 "%\+[a-zA-Z_#*-0-9][a-zA-Z0-9_]*" contains=tfVar
syn match  tfIdentifier2 "\${[a-zA-Z_#*-0-9][a-zA-Z0-9_]*}" contains=tfWorld
syn match  tfIdentifier4 "%\([#*]\|\<R\>\|\<-\=L\=\d\+\>\|\<P\d\+\>\)" contains=tfReadonly
syn match  tfIdentifier5 "\${\([#*]\|\<R\>\|\<-\=L\=\d\+\>\|\<P\d\+\>\)}" contains=tfReadonly

" Function names
syn keyword tfFunctions  ascii char columns echo filename ftime fwrite getopts 
syn keyword tfFunctions  getpid idle kbdel kbgoto kbhead kblen kbmatch kbpoint
syn keyword tfFunctions  kbtail kbwordleft kbwordright keycode lines mod
syn keyword tfFunctions  moresize pad rand read regmatch send strcat strchr
syn keyword tfFunctions  strcmp strlen strncmp strrchr strrep strstr substr
syn keyword tfFunctions  systype time tolower toupper

syn keyword tfStatement  addworld bamf beep bind break cat changes connect  contained
syn keyword tfStatement  dc def dokey echo edit escape eval export expr fg for  contained
syn keyword tfStatement  gag getfile grab help hilite histsize hook if input  contained
syn keyword tfStatement  kill lcd let list listsockets listworlds load  contained
syn keyword tfStatement  localecho log nohilite not partial paste ps purge  contained
syn keyword tfStatement  purgeworld putfile quit quote recall recordline save  contained
syn keyword tfStatement  saveworld send sh shift sub substitute  contained
syn keyword tfStatement  suspend telnet test time toggle trig trigger unbind  contained
syn keyword tfStatement  undef undefn undeft unhook  untrig unworld  contained
syn keyword tfStatement  version watchdog watchname while world  contained

" Hooks
syn keyword tfHook  ACTIVITY BACKGROUND BAMF CONFAIL CONFLICT CONNECT DISCONNECT
syn keyword tfHook  KILL LOAD LOADFAIL LOG LOGIN MAIL MORE PENDING PENDING
syn keyword tfHook  PROCESS PROMPT PROXY REDEF RESIZE RESUME SEND SHADOW SHELL
syn keyword tfHook  SIGHUP SIGTERM SIGUSR1 SIGUSR2 WORLD

" Conditional
syn keyword tfConditional  if endif then else elseif  contained

" Repeat
syn keyword tfRepeat  while do done repeat for  contained

" Statement
syn keyword tfStatement  break quit contained

" Operator
syn match tfOperator  "[-+=?:&|!]" 
syn match tfOperator  "[^/%]\*"hs=s+1 
syn match tfOperator  "/[^*~@]"he=e-1 
syn match tfOperator  ":=" 

" Relational
syn match tfRelation  "&&" 
syn match tfRelation  "||"
syn match tfRelation  "[<>/!=]="
syn match tfRelation  "[<>]"
syn match tfRelation  "[!=]\~"
syn match tfRelation  "[=!]/"

" Include
syn keyword  tfInclude require load save loaded contained

" Define
syn keyword  tfDefine bind unbind def undef undefn undefn purge hook unhook trig untrig  contained
syn keyword  tfDefine set unset setenv  contained

" Todo
syn keyword  tfTodo TODO Todo todo  contained

" SpecialChar
syn match tfSpecialChar "\\[abcfnrtyv\\]" contained
syn match tfSpecialChar "\\\d\{3}" contained contains=tfOctalError
syn match tfSpecialChar "\\x[0-9a-fA-F]\{2}" contained

syn match tfOctalError "[89]" contained

" Comment
syn region tfComment		start="^;" end="$"  contains=tfTodo

" String
syn region tfString   oneline matchgroup=None start=+'+  skip=+\\\\\|\\'+  end=+'+ contains=tfIdentifier1,tfSpecialChar,tfEscape
syn region tfString   matchgroup=None start=+"+  skip=+\\\\\|\\"+  end=+"+ contains=tfIdentifier1,tfSpecialChar,tfEscape

" Number
syn match tfNumber  "-\=\<\d\+\>"

" Float
syn match tfFloat  "\(-\=\<\d+\|-\=\)\.\d\+\>"

syn match tfParentError "[)}\]]" 

" Parents
syn region tfParent1 matchgroup=Delimiter start="(" end=")" contains=ALLBUT,tfReadonly
syn region tfParent2 matchgroup=Delimiter start="$\+(" end=")" contains=ALL 
syn region tfParent3 matchgroup=Delimiter start="$\+\[" end="\]" contains=ALL
syn region tfIdentifier3 matchgroup=Delimiter start="%*{" end="}" contains=ALL
syn match tfEndCommand "%\{-}; \{-}\\"

" Types

syn match tfType "/[a-zA-Z_~@][a-zA-Z0-9_]*" contains=tfConditional,tfRepeat,tfStatement,tfInclude,tfDefine,tfStatement

" Catch /quote .. '
syn match tfQuotes "/quote .\{-}'" contains=ALLBUT,tfString
" Catch $(/escape   )
syn match tfEscape "(/escape .*)" 

if !exists("did_f_syntax_inits")
  let did_tf_syntax_inits = 1
  " The default methods for highlighting.  Can be overridden later
  hi link tfComment                   Comment
  hi link tfString                    String
  hi link tfNumber                    Number
  hi link tfFloat                     Float
  hi tfIdentifier1 guifg=DarkGray ctermfg=Brown
  hi tfIdentifier2 guifg=DarkGray ctermfg=Brown
  hi tfIdentifier3 guifg=DarkGray ctermfg=Brown
  hi tfIdentifier4 guifg=DarkGray ctermfg=Brown
  hi tfIdentifier5 guifg=DarkGray ctermfg=Brown
  hi link tfFunctions                 Function
  hi link tfRepeat                    Repeat 
  hi link tfConditional               Conditional 
  hi link tfLabel                     Label
  hi link tfStatement                 Statement
  hi link tfType                      Type
  hi link tfInclude                   Include
  hi link tfDefine                    Define
  hi link tfSpecialChar               SpecialChar
  hi link tfParentError	              Error
  hi link tfTodo                      Todo
  hi link tfEndCommand		      Delimiter		
  hi tfRelation guifg=SeaGreen ctermfg=DarkGreen
  hi tfOperator guifg=SeaGreen ctermfg=DarkGreen
  hi tfVar guifg=Red ctermfg=DarkRed
  hi tfWorld guifg=Red ctermfg=DarkRed
  hi tfReadonly guifg=Red ctermfg=DarkRed
  hi tfHook guifg=Red ctermfg=DarkRed
endif
 
let b:current_syntax = "tf"

" vim: ts=8
