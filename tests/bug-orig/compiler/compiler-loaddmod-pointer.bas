; compiler-loaddmod-pointer (#23) - loaddmod switches the busy pointer on
; and calls pointeron only on the path that unloads an old module: without
; blitz2:defaultdbug at start there is none, and after "Load Debug Module"
; the pointer stays busy.  tedstub calls the menu item (9) with
; blitz2:dbgmod and logs the pointer state.  loaddmod only loads the file
; (LoadSeg) and keeps it, so any load file is a module here: the build's
; wbrun (amiga/wbrun.c), not the CD's DEFAULTDBUG.
; Correct: the pointer is normal again after the menu item.
; @fixed #23
; @pass remove=blitz2/defaultdbug
; @pass file=blitz2/dbgmod|@WBRUN@
; @pass jobline=menu=j001|9|blitz2:dbgmod
End
