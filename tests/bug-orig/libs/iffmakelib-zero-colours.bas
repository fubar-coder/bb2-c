; iffmakelib-zero-colours (#182) - makecolor counts the colours with
; SUBQ #1/DBF: a palette of 0 colours (InitPalette p,0 gives a valid
; palette with count 0) writes 65536 colours, 3 x 65536 bytes, into the
; 4096 byte buffer of the library and over what follows it.
; SaveBitmap with an empty palette is a programming error; its error
; check (bitchk2) tests only the palette's colour pointer. Runtime error
; checking on (the debugger).
; Here the file cannot be created (a directory that does not exist), so
; that the original stops at the Open with "Can't open File" before it
; writes the colours (otherwise it crashes): it shows that no check
; stopped the command before.
; Correct: the error check stops SaveBitmap with "Uninitialized Palette"
; (the message of a palette without colours).
; @fixed #182
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
BitMap 0,32,2,1
InitPalette 0,0
NPrint "save"
SaveBitmap 0,"RAM:bo182-nodir/p.iff",0
NPrint "saved"
End
