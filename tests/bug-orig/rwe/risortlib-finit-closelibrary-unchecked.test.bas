; risortlib-finit-closelibrary-unchecked (#824) - init opens
; utility.library and keeps the result; finit closes it without testing
; it.  Kickstart 1.x has no utility.library: finit calls CloseLibrary(0),
; which exec 1.x does not catch (it jumps to the close vector of a library
; at address 0) - a crash at the end of every program with the library.
; Here StringSortDir (does nothing else) brings the library in.
; What the crash leaves on the screen depends on the memory behind the
; program, so the program does not let it happen: it patches exec's
; CloseLibrary (SetFunction, LVO -414) with a stub that hangs in a loop
; when a1 is 0 and goes on to the original otherwise.  The stub is never
; freed: finit runs after End.  The call with a1 = 0 is seen as the hang
; (session timeout); a program that ends normally never made it.
; Correct: the program ends normally, also on Kickstart 1.3.
DEFTYPE .l

!TestCase{finit_closelibrary}
  code=AllocMem_(32,$10001)
  !Require{code<>0,"memory"}
  Poke.w code,$B2FC : Poke.w code+2,0                          ; cmpa.w #0,a1
  Poke.w code+4,$6602                                          ; bne.s +2
  Poke.w code+6,$60FE                                          ; bra.s * (the hang)
  Poke.w code+8,$2F39 : Poke.l code+10,code+16                 ; move.l old,-(sp)
  Poke.w code+14,$4E75                                         ; rts
  Poke.l code+16,0                                             ; old
  sysb=Peek.l(4)
  Forbid_
  o=SetFunction_(sysb,-414,code)
  Poke.l code+16,o
  Permit_
  StringSortDir 0
  !Check{1,"done"}
!EndTestCase
