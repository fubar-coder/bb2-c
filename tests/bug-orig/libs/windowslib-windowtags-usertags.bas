; windowslib-windowtags-usertags (#316) - the user tags of WindowTags are
; kept in a static buffer; the form without tags writes no TAG_DONE and
; takes over the tags the last WindowTags with tags left there.
; On the Workbench screen: WindowTags without tags (the buffer is still
; empty), then one with position and size tags, then again without tags.
; Correct: the last window opens like the first, not at 100,100 with
; 120x60.
; @configs clib
DEFTYPE .l
WbToScreen 0
DefaultIDCMP $400|$100|8|$40|$20
WindowTags 3,$000E,"Plain"
DefaultOutput
x1=WindowX : y1=WindowY : b1=WindowWidth : h1=WindowHeight
CloseWindow 3
WindowTags 2,$000E,"Tags",$80000064,100,$80000065,100,$80000066,120,$80000067,60
DefaultOutput
NPrint "tags ",WindowX," ",WindowY," ",WindowWidth," ",WindowHeight
CloseWindow 2
WindowTags 3,$000E,"Plain again"
DefaultOutput
x3=WindowX : y3=WindowY : b3=WindowWidth : h3=WindowHeight
NPrint "again ",x3," ",y3," ",b3," ",h3
NPrint "same as first ",x1=x3 AND y1=y3 AND b1=b3 AND h1=h3
CloseWindow 3
End
