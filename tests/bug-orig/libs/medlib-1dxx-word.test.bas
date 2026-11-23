; medlib-1dxx-word (#1066) - the pattern break 1Dxx (to line xx of the next block)
; adds 1 to the whole word of d4 and stores it as the next line, as Bxx
; does with the position (#203): d4 is cleared once per line and then
; loaded bytewise per track, its upper byte is left over from a track
; before it in the same line.  The module here (generated) has 19 08
; (sample offset: d4 = $0800) in track 0 and 1D 01 in track 1 of line 0:
; the next line is $0801, and since 1Dxx keeps the line at the change of
; the block, the line word of the module (+44, mmd_pline) is $0801 - a
; line far behind the block of 2 lines, whose notes are read from the
; memory behind it.  PlayMed plays one line (the first PlayMed after
; StartMedModule starts a line).
; Correct: the break goes to line 1 of the next position (1).
DEFTYPE .l

!TestCase{break_line}
  LoadMedModule 0,"CORPUS:1dxx.med"
  d=Peek.l(Addr MedModule(0)+8)
  StartMedModule 0
  PlayMed
  pos=Peek.w(d+46) AND $FFFF
  blk=Peek.w(d+42) AND $FFFF
  lin=Peek.w(d+44) AND $FFFF
  StopMed
  !CheckEq{pos,1,"position"}
  !CheckEq{blk,0,"block"}
  !CheckEq{lin,1,"line"}
!EndTestCase
