; windowslib-taglist-ueberlauf (#317) - WindowTags Window#,Flags,Title$,
; &TagList copies the program's tag list up to and including TAG_DONE
; into the static buffer usertags (32 longs = 16 pairs) without a limit;
; a longer list overwrites what follows the buffer in the library
; (bmapbase, then the code of WindowFont).
; Here: a list of 21 pairs - 18 TAG_IGNORE (data $4AFC4AFC, ILLEGAL),
; WA_Left 100, WA_Width 120, TAG_DONE - on the Workbench screen, then
; WindowFont with a style.  The program sets its own trap handler
; (tc_TrapCode of its task): an illegal instruction (exception 4)
; continues at the label in ar+8; any other exception goes on to the old
; handler.
; Correct: the window is at x = 100, 120 wide, and WindowFont works.  The
; original runs into the TAG_IGNORE data where WindowFont's code was.
; @fixed #317
; @configs clib
; @pass timeout=60
DEFTYPE .l
WbToScreen 0
LoadFont 0,"topaz.font",8
Dim t.l(43)
For i=0 To 34 Step 2 : t(i)=1 : t(i+1)=$4AFC4AFC : Next
t(36)=$80000064 : t(37)=100 : t(38)=$80000066 : t(39)=120
WindowTags 0,$000E,"Tags",&t(0)
DefaultOutput
NPrint "x ",WindowX," width ",WindowWidth
w=Peek.l(Peek.l(4)+276)
o=Peek.l(w+50)
Poke.l ?ar+4,o
Poke.l ?ar+8,?rec
Poke.l w+50,?th
WindowFont 0,0
Poke.l w+50,o
DefaultOutput
NPrint "font ok"
CloseWindow 0
End

rec:
  Poke.l w+50,o
  DefaultOutput
  NPrint "illegal instruction"
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
