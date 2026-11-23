; blitzoslib-spotbox-no-page (#83) - SpotBox has no page check (the other
; Blitz mode commands raise "UnInitialised Page" for a page whose memory
; size is 0: no InitPage, FreePage, or 0 boxes) and counts the boxes down
; with DBRA from numboxes-1: with 0 boxes it scans 65536 entries behind
; the page's memory.
; Here runtime error checking is on and the page has 0 boxes.
; The statement neverrun is never called: its LoadShape makes blitz2
; allocate the shape objects (#335).
; Correct: the error check refuses the page: "UnInitialised Page".
DEFTYPE .l

Statement neverrun{}
  LoadShape 0,"x"
End Statement

!TestCase{no_page}
  BitMap 0,320,200,2
  InitPage 0,0,0
  BLITZ
  !ExpectError{"UnInitialised Page","refused"}
  n=SpotBox
  AMIGA
  !Check{0,"not_refused"}
!EndTestCase
