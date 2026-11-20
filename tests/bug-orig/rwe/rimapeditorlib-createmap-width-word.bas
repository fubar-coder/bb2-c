; rimapeditorlib-createmap-width-word (issue to be filed) - CreateMap
; computes the size of the data as (width * 2 as a word) * height
; (ASL.w, MULU.w): a width above 32767 loses the upper bit, so a map
; 40000 wide gets 14464 bytes per row instead of 80000.  MapDataSize and
; MapDataAdr(x,y) compute the same word, but MapDataGrab fills width x
; height words and MapCanDraw reads the word at x * 2 as a long: both
; run up to 64 KB per row behind the data.  The width is the
; programmer's; the original has no error check area.  Here with runtime
; error checking.
; Correct: the error check refuses the width of 40000: "Map width above
; 32767".
; @rwe rimapeditorlib
; @pass runtime_errors=on
DEFTYPE .l
CreateMap 0,40000,1,16,16,"w"
If MapDataAdr=0 Then NPrint "no data" : End
s=MapDataSize
w=MapObjWidth AND $FFFF
NPrint "data ",s," bytes for ",w," words"
End
