" Menu Translations:	Espa�ol
" Maintainer:		Eduardo F. Amatria <eferna1@platea.pntic.mec.es>
" Last Change:		2001 Sep 02

" Quit when menu translations have already been done.
if exists("did_menu_trans")
  finish
endif
let did_menu_trans = 1

scriptencoding iso-8859-1

" Help menu
menutrans &Help			Ay&uda
menutrans &Overview<Tab><F1>	&Principal<Tab><F1>
menutrans &User\ Manual		&Manual\ del\ usuario
menutrans &How-to\ links	&Enlaces\ a\ �C�mo\.\.\.?
menutrans &GUI			&Interfaz\ gr�fica
menutrans &Credits		&Reconocimientos
menutrans Co&pying		&Copyright
menutrans &Find\.\.\.		&Buscar\.\.\.
menutrans &Version		&Versi�n
menutrans &About		&Acerca\ de\.\.\.

" File menu
menutrans &File				&Archivo
menutrans &Open\.\.\.<Tab>:e		&Abrir\.\.\.<Tab>:e
menutrans Sp&lit-Open\.\.\.<Tab>:sp	A&brir\ en\ otra\ ventana\.\.\.<Tab>:sp
menutrans &New<Tab>:enew		&Nuevo<Tab>:enew
menutrans &Close<Tab>:close		&Cerrar<Tab>:close
menutrans &Save<Tab>:w			&Guardar<Tab>:w
menutrans Save\ &As\.\.\.<Tab>:sav	Guardar\ &como\.\.\.<Tab>:sav
menutrans Split\ &Diff\ with\.\.\.	&Mostrar\ diferencias\ con\.\.\.
menutrans Split\ Patched\ &By\.\.\.	Mostrar\ &parcheado\ por\.\.\.
menutrans &Print			&Imprimir
menutrans Sa&ve-Exit<Tab>:wqa		Gua&rdar\ y\ salir<Tab>:wqa
menutrans E&xit<Tab>:qa			&Salir<Tab>:qa

" Edit menu
menutrans &Edit				&Editar
menutrans &Undo<Tab>u			&Deshacer<Tab>u
menutrans &Redo<Tab>^R			&Rehacer<Tab>^R
menutrans Rep&eat<Tab>\.		Repe&tir<Tab>\.
menutrans Cu&t<Tab>"+x			Cor&tar<Tab>"+x
menutrans &Copy<Tab>"+y			&Copiar<Tab>"+y
menutrans &Paste<Tab>"+P		&Pegar<Tab>"+P
menutrans Put\ &Before<Tab>[p		Poner\ &antes<Tab>[p
menutrans Put\ &After<Tab>]p		Poner\ &despu�s<Tab>]p
if has("win32") || has("win16")
  menutrans &Delete<Tab>x		S&uprimir<Tab>x
endif
menutrans &Select\ all<Tab>ggVG		&Seleccionar\ todo<Tab>ggVG
menutrans &Find\.\.\.			&Buscar\.\.\.
menutrans Find\ and\ Rep&lace\.\.\.     Buscar\ y\ R&eemplazar\.\.\.
menutrans Settings\ &Window		&Ventana\ de\ opciones

" Edit/Global Settings
menutrans &Global\ Settings		Opciones\ &globales

menutrans Toggle\ Pattern\ &Highlight<Tab>:set\ hls!	Activar/Desactivar\ &realzado\ de\ patrones<Tab>:set\ hls!
menutrans Toggle\ &Ignore-case<Tab>:set\ ic!		Activar/Desactivar\ &ignorar\ la\ caja<Tab>:set\ ic!
menutrans Toggle\ &Showmatch<Tab>:set\ sm!		Activar/Desactivar\ &mostrar\ coincidencias<Tab>:set\ sm!

menutrans &Context\ lines		L�neas\ de\ &contexto

menutrans &Virtual\ Edit		Edici�n\ &virtual
menutrans Never				Nunca
menutrans Block\ Selection		Selecci�n\ de\ bloque
menutrans Insert\ mode			Modo\ insertar
menutrans Block\ and\ Insert		Bloque\ e\ insertar
menutrans Always			Siempre

menutrans Toggle\ Insert\ &Mode<Tab>:set\ im!	Activar/Desactivar\ modo\ de\ in&serci�n<Tab>:set\ im!

menutrans Search\ &Path\.\.\.		Ruta\ de\ &b�squeda\.\.\.

menutrans Ta&g\ Files\.\.\.		Ficheros\ de\ �ta&gs�\.\.\.

" GUI options
menutrans Toggle\ &Toolbar		Ocultar/Mostrar\ barra\ de\ &herramientas
menutrans Toggle\ &Bottom\ Scrollbar	Ocultar/Mostrar\ barra\ de\ desplazamiento\ &inferior
menutrans Toggle\ &Left\ Scrollbar	Ocultar/Mostrar\ barra\ de\ desplazamiento\ i&zquierda
menutrans Toggle\ &Right\ Scrollbar	Ocultar/Mostrar\ barra\ de\ desplazamiento\ &derecha

let g:menutrans_path_dialog = "Introduzca la ruta de b�squeda para los ficheros.\nSepare los nombres de los directorios con una coma."
let g:menutrans_tags_dialog = "Introduzca los nombres de los fichero de tags.\nSepare los nombres con una coma."

" Edit/File Settings
menutrans F&ile\ Settings		Opciones\ del\ &fichero

" Boolean options
menutrans Toggle\ Line\ &Numbering<Tab>:set\ nu!	Activar/Desactivar\ &numeraci�n\ de\ l�neas<Tab>:set\ nu!
menutrans Toggle\ &List\ Mode<Tab>:set\ list!		Activar/Desactivar\ modo\ �&list�<Tab>:set\ list!
menutrans Toggle\ Line\ &Wrap<Tab>:set\ wrap!		Activar/Desactivar\ &doblado\ de\ l�neas<Tab>:set\ wrap!
menutrans Toggle\ W&rap\ at\ word<Tab>:set\ lbr!	Activar/Desactivar\ doblado\ en\ &palabra<Tab>:set\ lbr!
menutrans Toggle\ &expand-tab<Tab>:set\ et!		Activar/Desactivar\ &expansi�n\ de\ tabs<Tab>:set\ et!
menutrans Toggle\ &auto-indent<Tab>:set\ ai!		Activar/Desactivar\ &auto-sangrado<Tab>:set\ ai!
menutrans Toggle\ &C-indenting<Tab>:set\ cin!		Activar/Desactivar\ sangrado\ &C<Tab>:set\ cin!

" other options
menutrans &Shiftwidth			Anchura\ del\ &sangrado

menutrans Soft\ &Tabstop		&Tabulado\ suave

menutrans Te&xt\ Width\.\.\.		Anchura\ del\ te&xto\.\.\.
let g:menutrans_textwidth_dialog = "Introduzca la nueva anchura del texto (0 para desactivar el formateo): "

menutrans &File\ Format\.\.\.		&Formato\ del\ fichero\.\.\.
let g:menutrans_fileformat_dialog = "Seleccione el formato para escribir el fichero"

menutrans C&olor\ Scheme		Esquema\ de\ c&olores
menutrans Select\ Fo&nt\.\.\.		Seleccionar\ fue&nte\.\.\.

menutrans &Keymap	Asociaci�n\ de\ teclas
menutrans None		Ninguna

" Programming menu
menutrans &Tools			&Herramientas
menutrans &Jump\ to\ this\ tag<Tab>g^]	&Saltar\ a\ este\ �tag�<Tab>g^]
menutrans Jump\ &back<Tab>^T		Saltar\ &atr�s<Tab>^T
menutrans Build\ &Tags\ File		&Generar\ fichero\ de\ �tags�\
menutrans &Folding			&Plegado
menutrans &Make<Tab>:make		Ejecutar\ �&Make�<Tab>:make
menutrans &List\ Errors<Tab>:cl		&Lista\ de\ errores<Tab>:cl
menutrans L&ist\ Messages<Tab>:cl!	L&ista\ de\ mensajes<Tab>:cl!
menutrans &Next\ Error<Tab>:cn		&Error\ siguiente<Tab>:cn
menutrans &Previous\ Error<Tab>:cp	Error\ p&revio<Tab>:cp
menutrans &Older\ List<Tab>:cold	Lista\ de\ &viejos\ a\ nuevos<Tab>:cold
menutrans N&ewer\ List<Tab>:cnew	Lista\ de\ &nuevos\ a\ viejos<Tab>:cnew
menutrans Error\ &Window		Ven&tana\ de\ errores
menutrans &Set\ Compiler		Fi&jar\ el\ compilador
menutrans &Convert\ to\ HEX<Tab>:%!xxd	Convertir\ a\ &HEX<Tab>:%!xxd
menutrans Conve&rt\ back<Tab>:%!xxd\ -r	&Convertir\ al\ anterior<Tab>:%!xxd\ -r

" Tools.Fold Menu
menutrans &Enable/Disable\ folds<Tab>zi		&Activar/Desactivar\ pliegues<Tab>zi
menutrans &View\ Cursor\ Line<Tab>zv		&Ver\ l�nea\ del\ cursor<Tab>zv
menutrans Vie&w\ Cursor\ Line\ only<Tab>zMzx	Ve&r\ s�lo\ la\ l�nea\ del\ cursor<Tab>zMzx
menutrans C&lose\ more\ folds<Tab>zm		C&errar\ m�s\ pliegues<Tab>zm
menutrans &Close\ all\ folds<Tab>zM		&Cerrar\ todos\ los\ pliegues<Tab>zM
menutrans O&pen\ more\ folds<Tab>zr		Abrir\ &m�s\ pliegues<Tab>zr
menutrans &Open\ all\ folds<Tab>zR		&Abrir\ todos\ los\ pliegues<Tab>zR
" fold method
menutrans Fold\ Met&hod				&M�todo\ de\ plegado
" create and delete folds
menutrans Create\ &Fold<Tab>zf			Crear\ &pliegue<Tab>zf
menutrans &Delete\ Fold<Tab>zd			&Suprimir\ pliegue<Tab>zd
menutrans Delete\ &All\ Folds<Tab>zD		Suprimir\ &todos\ los\ pligues<Tab>zD
" moving around in folds
menutrans Fold\ column\ &width			A&nchura\ de\ columna\ del\ pliegue

" Tools.Diff Menu
menutrans &Update	&Actualizar
menutrans &Get\ Block	&Obtener\ bloque
menutrans &Put\ Block	&Poner\ bloque

"Tools.Error Menu
menutrans &Update<Tab>:cwin	&Actualizar
menutrans &Open<Tab>:copen	A&brir
menutrans &Close<Tab>:cclose	&Cerrar

" Names for buffer menu.
menutrans &Buffers		&Buffers
menutrans &Refresh\ menu	&Refrescar\ men�
menutrans &Delete		&Suprimir
menutrans &Alternate		&Alternar
menutrans &Next			Si&guiente
menutrans &Previous		&Previo
let g:menutrans_no_file = "[Sin fichero]"

" Window menu
menutrans &Window			&Ventana
menutrans &New<Tab>^Wn			&Nueva<Tab>^Wn
menutrans S&plit<Tab>^Ws		&Dividir<Tab>^Ws
menutrans Sp&lit\ To\ #<Tab>^W^^	D&ividir\ a\ #<Tab>^W^^
menutrans Split\ &Vertically<Tab>^Wv    Dividir\ &verticalmente<Tab>^Wv
menutrans Split\ File\ E&xplorer	&Abrir\ Explorador\ de\ ficheros
menutrans &Close<Tab>^Wc		&Cerrar<Tab>^Wc
menutrans Close\ &Other(s)<Tab>^Wo	Cerrar\ &otra(s)<Tab>^Wo
menutrans Move\ &To			Mov&er\ a
menutrans &Top<Tab>^WK			&Arriba<Tab>^WK
menutrans &Bottom<Tab>^WJ		A&bajo<Tab>^WJ
menutrans &Left\ side<Tab>^WH		Lado\ &izquierdo<Tab>^WH
menutrans &Right\ side<Tab>^WL		Lado\ &derecho<Tab>^WL
menutrans Rotate\ &Up<Tab>^WR		&Rotar\ hacia\ arriba<Tab>^WR
menutrans Rotate\ &Down<Tab>^Wr		Rotar\ hacia\ a&bajo<Tab>^Wr
menutrans &Equal\ Size<Tab>^W=		Mismo\ &tama�o<Tab>^W=
menutrans &Max\ Height<Tab>^W_		Altura\ &m�xima<Tab>^W_
menutrans M&in\ Height<Tab>^W1_		Altura\ m�&nima<Tab>^W1_
menutrans Max\ &Width<Tab>^W\|		Anchura\ m�&xima<Tab>^W\|
menutrans Min\ Widt&h<Tab>^W1\|		Anc&hura\ m�nima<Tab>^W1\|

" The popup menu
menutrans &Undo			&Deshacer
menutrans Cu&t			Cor&tar
menutrans &Copy			&Copiar
menutrans &Paste		&Pegar
menutrans &Delete		&Borrar
menutrans Select\ Blockwise	Seleccionar\ por\ bloque
menutrans Select\ &Word		Seleccionar\ &palabra
menutrans Select\ &Line		Seleccionar\ una\ &l�nea
menutrans Select\ &Block	Seleccionar\ un\ &bloque
menutrans Select\ &All		Seleccionar\ &todo

" The GUI toolbar (for Win32, GTK, Motif or Athena)
if has("toolbar")
  if exists("*Do_toolbar_tmenu")
    delfun Do_toolbar_tmenu
  endif
  fun Do_toolbar_tmenu()
    tmenu ToolBar.Open		Abrir fichero
    tmenu ToolBar.Save		Guardar fichero
    tmenu ToolBar.SaveAll	Guardar todos los ficheros
    tmenu ToolBar.Print		Imprimir
    tmenu ToolBar.Undo		Deshacer
    tmenu ToolBar.Redo		Rehacer
    tmenu ToolBar.Cut		Cortar
    tmenu ToolBar.Copy		Copiar
    tmenu ToolBar.Paste		Pegar
    tmenu ToolBar.Find		Buscar...
    tmenu ToolBar.FindNext	Buscar siguiente
    tmenu ToolBar.FindPrev	Buscar precedente
    tmenu ToolBar.Replace	Buscar y reemplazar
    tmenu ToolBar.New		Ventana nueva
    tmenu ToolBar.WinSplit	Dividir ventana
    tmenu ToolBar.WinMax	Ventana m�xima
    tmenu ToolBar.WinMin	Ventana m�nima
    tmenu ToolBar.WinVSplit	Dividir verticalmente
    tmenu ToolBar.WinMaxWidth	Anchura m�xima
    tmenu ToolBar.WinMinWidth	Anchura m�nima
    tmenu ToolBar.WinClose	Cerrar ventana
    tmenu ToolBar.LoadSesn	Cargar sesi�n
    tmenu ToolBar.SaveSesn	Guardar sesi�n
    tmenu ToolBar.RunScript	Ejecutar un �script�
    tmenu ToolBar.Make		Ejecutar �Make�
    tmenu ToolBar.Shell		Abrir una �Shell�
    tmenu ToolBar.RunCtags	Generar un fichero de �tags�
    tmenu ToolBar.TagJump	Saltar a un �tag�
    tmenu ToolBar.Help		Ayuda
    tmenu ToolBar.FindHelp	Buscar en la ayuda...
  endfun
endif

" Syntax menu
menutrans &Syntax			&Sintaxis
menutrans Set\ '&syntax'\ only		Activar\ s�lo\ �sintaxis�
menutrans Set\ '&filetype'\ too		Activar\ tambi�n\ �tipo\ de\ fichero�
menutrans &Off				&Desactivar
menutrans &Manual			&Manual
menutrans A&utomatic			A&utom�tica
menutrans on/off\ for\ &This\ file	Activar/Desactivar\ en\ es&te\ fichero
menutrans Co&lor\ test			&Prueba\ del\ color
menutrans &Highlight\ test		Prueba\ del\ &realzado
menutrans &Convert\ to\ HTML		&Convertir\ en\ HTML

" Find Help dialog text
let g:menutrans_help_dialog = "Introduzca un mandato o palabra para obtener ayuda;\n\nAnteponga i_ para mandatos de entrada (e.g.: i_CTRL-X)\nAnteponga c_ para mandatos de la l�nea de mandatos (e.g.: c_<Del>)\nAnteponga ` para un nombre de opci�n 8(e.g.: `shiftwidth`)"