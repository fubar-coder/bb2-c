; blitzoslib-boxstring-all-spaces (#85) - BoxString strips the trailing
; spaces of the answer backwards without stopping at its start: for an
; answer of spaces only behind a prompt that ends in a space, the loop
; runs into the prompt, the length goes negative and 65536-n bytes are
; copied into the string buffer (the cell has one of 300000 bytes, so the
; copy stays inside it).
; Here a string box 160 pixels wide (20 bytes) with the prompt "Name: ",
; never edited: the answer is 13 spaces.
; The statement neverrun is never called: its LoadShape makes blitz2
; allocate the shape objects (#335).
; Correct: the answer is the empty string.
DEFTYPE .l

Statement neverrun{}
  LoadShape 0,"x"
End Statement

!TestCase{all_spaces}
  BitMap 0,320,200,2
  InitPage 0,0,1
  AddBox 0,0,0,0,160,12,4,"Name: "
  BLITZ
  a$=BoxString(0)
  AMIGA
  n=Len(a$)
  !CheckEq{n,0,"length"}
!EndTestCase
