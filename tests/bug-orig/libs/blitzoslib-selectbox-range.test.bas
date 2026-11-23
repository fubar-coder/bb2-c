; blitzoslib-selectbox-range (#82) - SelectBox Box# does not check the
; box number against the number of boxes (the other box commands raise
; "Illegal Box# For Page"): a number behind the table selects and draws
; an entry of whatever memory lies there.
; Here runtime error checking is on, the page has 2 boxes: SelectBox 1
; and SelectBox -1 (deselect) are valid, then SelectBox 5 follows.
; The statement neverrun is never called: its LoadShape makes blitz2
; allocate the shape objects (#335).
; Correct: the valid numbers pass, the error check refuses 5: "Illegal
; Box# For Page".
DEFTYPE .l

Statement neverrun{}
  LoadShape 0,"x"
End Statement

!TestCase{box_range}
  BitMap 0,320,200,2
  InitPage 0,0,2
  AddBox 0,0,0,0,80,12,2,"A"
  AddBox 0,1,0,20,80,12,2,"B"
  BLITZ
  BitMapOutput 0
  SelectBox 1
  s1=BoxSelected
  SelectBox -1
  s2=BoxSelected
  AMIGA
  DefaultOutput
  !CheckEq{s1,1,"valid_selected"}
  !CheckEq{s2,-1,"valid_deselected"}
  BLITZ
  BitMapOutput 0
  !ExpectError{"Illegal Box# For Page","refused"}
  SelectBox 5
  AMIGA
  DefaultOutput
  !Check{0,"not_refused"}
!EndTestCase
