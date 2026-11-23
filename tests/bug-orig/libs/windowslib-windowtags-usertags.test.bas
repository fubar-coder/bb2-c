; windowslib-windowtags-usertags (#316) - the user tags of WindowTags are
; kept in a static buffer; the form without tags writes no TAG_DONE and
; takes over the tags the last WindowTags with tags left there.
; Here: on the Workbench screen: WindowTags without tags (the buffer is
; still empty), then one with position and size tags, then again without
; tags.
; Correct: the last window opens like the first, not at 100,100 with
; 120x60.
DEFTYPE .l

!TestCase{plain_after_tags}
  WbToScreen 0
  DefaultIDCMP $400|$100|8|$40|$20
  WindowTags 3,$000E,"Plain"
  x1=WindowX : y1=WindowY : b1=WindowWidth : h1=WindowHeight
  CloseWindow 3
  WindowTags 2,$000E,"Tags",$80000064,100,$80000065,100,$80000066,120,$80000067,60
  !CheckEq{WindowX,100,"tags_x"}
  !CheckEq{WindowY,100,"tags_y"}
  !CheckEq{WindowWidth,120,"tags_width"}
  !CheckEq{WindowHeight,60,"tags_height"}
  CloseWindow 2
  WindowTags 3,$000E,"Plain again"
  x3=WindowX : y3=WindowY : b3=WindowWidth : h3=WindowHeight
  !Check{x1=x3 AND y1=y3 AND b1=b3 AND h1=h3,"same_as_first"}
  CloseWindow 3
!EndTestCase
