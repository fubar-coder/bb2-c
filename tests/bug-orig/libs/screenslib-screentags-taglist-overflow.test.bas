; screenslib-screentags-taglist-overflow (#249) - ScreenTags
; Screen#,Title$,&TagList copies the program's tag list up to and
; including TAG_END into the static buffer usertags (32 longs = 15 tags
; and TAG_END) without a limit; a longer list overwrites what follows the
; buffer (in the original the error check area, without the debugger the
; code of the next library of the program).
; Here: a list of 123 tags - 120 TAG_IGNORE (data $4AFC4AFC, ILLEGAL),
; SA_Left 16, SA_Depth 2, TAG_END - then the screen's position and depth,
; CloseScreen and a FindScreen (code of screenslib and the libraries
; behind it).  The program sets its own trap handler (tc_TrapCode of its
; task): an illegal instruction (exception 4) continues at the label in
; ar+8; any other exception goes on to the old handler.
; Correct: the screen opens at x = 16 with depth 2, and the program goes
; on.
; The handler (labels, assembler and the data of its longs) stands in the
; case, behind a Goto that skips it; the label rec checks that the
; illegal instruction came and ends the program.
DEFTYPE .l

!TestCase{taglist_overflow}
  Dim t.l(250)
  For i=0 To 239 Step 2 : t(i)=1 : t(i+1)=$4AFC4AFC : Next
  t(240)=$80000021 : t(241)=16 : t(242)=$80000025 : t(243)=2 : t(244)=0
  w=Peek.l(Peek.l(4)+276)
  o=Peek.l(w+50)
  Poke.l ?ar+4,o
  Poke.l ?ar+8,?rec
  Poke.l w+50,?th
  s=ScreenTags(0,"Tags",&t(0))
  x=Peek.w(s+8) : d=Peek.b(s+189)
  CloseScreen 0
  WbToScreen 1
  FindScreen 2
  a$=Left$("abc",1)+Str$(Val("12"))
  ShowScreen 1
  Poke.l w+50,o
  !CheckEq{x,16,"screen_x"}
  !CheckEq{d,2,"screen_depth"}
  Goto fin

rec:
  Poke.l w+50,o
  !Check{0,"illegal_instruction"}
  End

th:
  CMP.l #4,(a7) : BNE thold
  ADDQ.l #4,a7
  MOVE.l ar+8(pc),2(a7)
  RTE
thold:
  MOVE.l ar+4(pc),-(a7)
  RTS

ar:
  Dc.l 0,0,0

fin:
!EndTestCase
