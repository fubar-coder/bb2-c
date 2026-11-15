; blitzoslib-deselectbox-selected (#94) - DeSelectBox redraws the selected
; box and ends input, prop and drag mode, but does not clear the page's
; selected box: BoxSelected still returns it (and BlitzEvent's keys still
; act on it).
; The command after End is never run: it makes blitz2 allocate the shape
; objects (#335).
; Correct: after DeSelectBox no box is selected (BoxSelected -1).
; @fixed #94
; @configs clib
BitMap 0,320,200,2
InitPage 0,0,1
AddBox 0,0,0,0,80,12,2,"OK"
BLITZ
BitMapOutput 0
SelectBox 0
a=BoxSelected
DeSelectBox
n=BoxSelected
AMIGA
DefaultOutput
NPrint "selected ",a
NPrint "after DeSelectBox ",n
End
LoadShape 0,"x"
