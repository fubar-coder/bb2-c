; blitzoslib-deselectbox-selected (#94) - DeSelectBox redraws the selected
; box and ends input, prop and drag mode, but does not clear the page's
; selected box: BoxSelected still returns it (and BlitzEvent's keys still
; act on it).
; The statement neverrun is never called: its LoadShape makes blitz2
; allocate the shape objects (#335).
; Correct: after DeSelectBox no box is selected (BoxSelected -1).
DEFTYPE .l

Statement neverrun{}
  LoadShape 0,"x"
End Statement

!TestCase{deselect}
  BitMap 0,320,200,2
  InitPage 0,0,1
  AddBox 0,0,0,0,80,12,2,"OK"
  BLITZ
  BitMapOutput 0
  SelectBox 0
  a=BoxSelected
  DeSelectBox
  n=BoxSelected
  AMIGA
  DefaultOutput
  !Require{a=0,"box_selected"}
  !CheckEq{n,-1,"after_deselect"}
!EndTestCase
