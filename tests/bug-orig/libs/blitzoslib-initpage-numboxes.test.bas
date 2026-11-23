; blitzoslib-initpage-numboxes (#1043) - InitPage computes the size of
; the box table as NumBoxes x 32 of the word sign-extended (EXT.l): for
; NumBoxes of 32768 or more the size is negative, a huge unsigned number,
; and the allocation fails (or behaves unexpectedly).  InitPage has no
; error check.
; Here runtime error checking is on: InitPage 0,0,32768, then the memory
; size of the page (+0 of the page object).
; The statement neverrun is never called: its LoadShape makes blitz2
; allocate the shape objects (#335).
; Correct: an error check refuses the number: "Illegal Number of Boxes",
; the command is not done.
DEFTYPE .l

Statement neverrun{}
  LoadShape 0,"x"
End Statement

!TestCase{numboxes}
  BitMap 0,320,200,2
  !ExpectError{"Illegal Number of Boxes","refused"}
  InitPage 0,0,32768
  !Check{0,"not_refused"}
!EndTestCase
