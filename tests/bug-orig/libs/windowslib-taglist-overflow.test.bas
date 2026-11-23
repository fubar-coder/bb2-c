; windowslib-taglist-overflow (#317) - WindowTags Window#,Flags,Title$,
; &TagList copies the program's tag list up to and including TAG_DONE
; into the static buffer usertags (32 longs = 16 pairs) without a limit;
; a longer list overwrites what follows the buffer in the library
; (bmapbase, then the code of WindowFont).
; Here: a list of 21 pairs - 18 TAG_IGNORE (data $4AFC4AFC, ILLEGAL),
; WA_Left 100, WA_Width 120, TAG_DONE - on the Workbench screen, then
; WindowFont with a style.  The program sets its own trap handler
; (tc_TrapCode of its task): an illegal instruction (exception 4)
; continues at the label tl_rec; any other exception goes on to the old
; handler.  The handler code and its data stand in the case, behind a
; jump.
; Correct: the window is at x = 100, 120 wide, and WindowFont works.  The
; original runs into the TAG_IGNORE data where WindowFont's code was: the
; handler records the failed check and ends the program.
DEFTYPE .l

!TestCase{tag_list_21_pairs}
  Goto tl_start
tl_th:
  CMP.l #4,(a7) : BNE tl_thold
  ADDQ.l #4,a7
  MOVE.l tl_ar+8(pc),2(a7)
  RTE
tl_thold:
  MOVE.l tl_ar+4(pc),-(a7)
  RTS
tl_ar:
  Dc.l 0,0,0
tl_start:
  WbToScreen 0
  LoadFont 0,"topaz.font",8
  Dim t.l(43)
  For i=0 To 34 Step 2 : t(i)=1 : t(i+1)=$4AFC4AFC : Next
  t(36)=$80000064 : t(37)=100 : t(38)=$80000066 : t(39)=120
  WindowTags 0,$000E,"Tags",&t(0)
  !CheckEq{WindowX,100,"window_x"}
  !CheckEq{WindowWidth,120,"window_width"}
  w=Peek.l(Peek.l(4)+276)
  o=Peek.l(w+50)
  Poke.l ?tl_ar+4,o
  Poke.l ?tl_ar+8,?tl_rec
  Poke.l w+50,?tl_th
  WindowFont 0,0
  Poke.l w+50,o
  !Check{1,"windowfont_returns"}
  CloseWindow 0
  Goto tl_done
tl_rec:
  Poke.l w+50,o
  !Check{0,"no_illegal_instruction"}
  End
tl_done:
!EndTestCase
