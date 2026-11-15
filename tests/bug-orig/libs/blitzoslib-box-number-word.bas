; blitzoslib-box-number-word (#81) - the address of a box (32 bytes) is
; computed with ASL #5 and ADDA as a word: from box 1024 on the offset is
; negative, from 2048 on it wraps to the boxes from 0 on.  InitPage
; allocates the table with a long product, so a page may have more than
; 1024 boxes.  BoxSelected divides the offset back as a word (LSR.w).
; Here a page of 2100 boxes: AddBox and ChangeBox on box 2050 (offset
; 65600, as a word 64: box 2 of the original), then BoxValue, SelectBox
; 2050 and BoxSelected in Blitz mode.  The page object is at
; Addr Page(0), its +4 is the address of the boxes (x at +0, range at
; +14).
; The command after End is never run: it makes blitz2 allocate the shape
; objects (#335).
; Correct: box 2050 gets the values (x 5, range 3), box 2 stays empty,
; the selected box is 2050.
; @fixed #81
; @configs clib
DEFTYPE .l
BitMap 0,320,200,2
InitPage 0,0,2100
b=Peek.l(Addr Page(0)+4)
AddBox 0,2050,5,6,80,12,2,"A|B|C"
ChangeBox 0,2050,2
BLITZ
BitMapOutput 0
v=BoxValue(0,2050)
SelectBox 2050
s=BoxSelected
AMIGA
DefaultOutput
NPrint "box 2050 x ",Peek.w(b+65600)," range ",Peek.w(b+65600+14)
NPrint "box 2 x ",Peek.w(b+64)
NPrint "value ",v
NPrint "selected ",s
End
LoadShape 0,"x"
