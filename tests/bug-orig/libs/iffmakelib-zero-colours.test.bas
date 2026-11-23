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
; (the message of a palette without colours).  The message is the tested
; behaviour, so the error is not announced with !ExpectError (see the
; configuration).
DEFTYPE .l

!TestCase{empty_palette}
  BitMap 0,32,2,1
  InitPalette 0,0
  SaveBitmap 0,"CORPUS:bo182-nodir/p.iff",0
  !Check{0,"saved"}
!EndTestCase
