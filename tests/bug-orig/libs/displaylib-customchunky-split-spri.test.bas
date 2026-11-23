; displaylib-customchunky-split-spri (#949) - the form of CustomChunky
; with a split position copies the chunky description of the CopList
; (cchunky..chc, +96) to the second one (cchunky2..chc2, +112) as 16
; bytes (MOVEM.l d0-d3): the last word, pad0 (0), overwrites spri (+126),
; the sprite resolution shift that DisplaySprite scales the x handle by.
; Amiga mode (the list is built in memory only, as in t164_display): a
; CopList with hires sprites ($800 in the type: spri 1), CustomChunky
; with a split position; spri before and after, and the second chunky
; description.
; Correct: spri stays 1; the second description is the first one's,
; split.
DEFTYPE .l

!TestCase{split_spri}
  InitCopList 2,44,40,$10801,0,0,200
  c=Addr CopList(2)
  !CheckEq{Peek.w(c+126),1,"spri_before"}
  CustomChunky 2,60,20,8,4,2,2
  !CheckEq{Peek.w(c+126),1,"spri_after"}
  !CheckEq{Peek.l(c+112)-Peek.l(c+4),418,"chunky2_offset"}
  !CheckEq{Peek.w(c+116),20,"chunky2_w116"}
  !CheckEq{Peek.w(c+118),8,"chunky2_w118"}
  !CheckEq{Peek.w(c+120),2,"chunky2_w120"}
  !CheckEq{Peek.w(c+122),2,"chunky2_w122"}
  !CheckEq{Peek.w(c+124),44,"chunky2_w124"}
!EndTestCase
