; polylib-renderpoly-colour-check (#422) - RenderPoly after RenderToChunky
; takes the colour through a table of 15 chunky colours without checking
; it: a colour above 14 reads the words behind the table.  Such a colour
; is a programming error, and the original has no error check for it,
; not even with runtime error checking.
; Here with runtime error checking: the chunky area of CopList 0
; (CustomChunky, never displayed), RenderToChunky 0, a triangle with
; colour 14 (no error), then with colour 15, in a short BLITZ ... AMIGA
; section (RenderPoly is Blitz mode only).
; Correct: the error check refuses colour 15: "Illegal Colour".
; The error is announced with !ExpectError; the original raises none (see
; the configuration).
DEFTYPE .l

!TestCase{colour_15}
  cs=CustomChunkySize(64,16)
  InitCopList 0,44,64,1,0,2,cs
  CustomChunky 0,0,8,64,16,1
  Dim p.w(20)
  p(0)=1 : p(1)=1 : p(2)=12 : p(3)=3 : p(4)=6 : p(5)=40
  a=&p(0)
  VWait
  BLITZ
  RenderToChunky 0
  RenderPoly 3,a,14
  !ExpectError{"Illegal Colour","refused"}
  RenderPoly 3,a,15
  VWait 2
  AMIGA
  !Check{0,"after"}
!EndTestCase
