; risortlib-finit-closelibrary-unchecked (#824) - init opens
; utility.library and keeps the result; finit closes it without testing
; it.  Kickstart 1.x has no utility.library: finit calls CloseLibrary(0),
; which exec 1.x does not catch (it jumps to the close vector of a library
; at address 0) - a crash at the end of every program with the library.
; Here StringSortDir (does nothing else) brings the library in.
; Correct: the program ends normally, also on Kickstart 1.3.
; @rwe risortlib
; @machine a500
StringSortDir 0
NPrint "done"
End
