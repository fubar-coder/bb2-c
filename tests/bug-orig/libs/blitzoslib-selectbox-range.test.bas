; blitzoslib-selectbox-range (#82) - SelectBox Box# does not check the
; box number against the number of boxes (the other box commands raise
; "Illegal Box# For Page"): a number behind the table selects and draws
; an entry of whatever memory lies there.
; The boxes are 32 bytes each (the table of 2 boxes is 64 bytes), so box 2
; is the first entry behind the table; the cell runs with inject's pad
; (64 zeroed bytes behind every allocation), which makes that entry a
; zeroed box (the 32 bytes lie inside the 64 of the pad) that the original
; selects and draws: the end is the same on every run.
; Here runtime error checking is on, the page has 2 boxes: SelectBox 1
; and SelectBox -1 (deselect) are valid, then SelectBox 2 follows.
; The statement neverrun is never called: its LoadShape makes blitz2
; allocate the shape objects (#335).
; Correct: the valid numbers pass, the error check refuses 2: "Illegal
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
  SelectBox 2
  AMIGA
  DefaultOutput
  !Check{0,"not_refused"}
!EndTestCase
