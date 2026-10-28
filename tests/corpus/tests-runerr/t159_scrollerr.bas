; t159_scrollerr - scrolllib: Scroll with a destination area that leaves
; the bitmap (error check: "Illegal Scroll Area")
BitMap 0,96,40,2
Use BitMap 0
Scroll 0,0,40,10,50,0
NPrint "before"
Scroll 0,0,50,10,60,0
NPrint "after"
End
