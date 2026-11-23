; rifnslib-settab-zero (#666) - FNSSetTab stores the tab size unchecked;
; a Chr$(9) in a printed text moves x to the next tab stop with DIVU
; tabsize: after FNSSetTab 0 a division by zero (the session ends in a
; timeout).  A tab size below 1 is an invalid argument; the original has
; no error check area.  Here with runtime error checking.  The font
; (built in memory, installed in slot 1) is 16 pixels wide and 8 high,
; its glyphs empty.
; Correct: the error check refuses the tab size: "FNSSetTab size must be
; at least 1".
DEFTYPE .l

!TestCase{tab_zero}
  f=AllocMem_(1024,$10001)
  !Require{f<>0,"memory"}
  Poke.l f,$464E532E
  Poke.w f+4,8 : Poke.w f+6,16 : Poke.w f+8,0 : Poke.w f+10,32
  Poke.b f+33,8
  BitMap 0,320,64,1
  r=InstallFNS(1,f)
  FNSOutput 0
  FNSClip 0,0,319,63
  FNSInk 1
  !ExpectError{"FNSSetTab size must be at least 1","refused"}
  FNSSetTab 0
  FNSPrint 1,0,0,"a"+Chr$(9)+"b"
  !Check{0,"not_refused"}
  RemoveFNS 1
  FreeMem_ f,1024
!EndTestCase
