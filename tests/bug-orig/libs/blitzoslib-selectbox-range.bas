; blitzoslib-selectbox-range (#82) - SelectBox Box# does not check the
; box number against the number of boxes (the other box commands raise
; "Illegal Box# For Page"): a number behind the table selects and draws
; an entry of whatever memory lies there.
; Here runtime error checking is on, the page has 2 boxes: SelectBox 1
; and SelectBox -1 (deselect) are valid, then SelectBox 5 follows.
; The command after End is never run: it makes blitz2 allocate the shape
; objects (#335).
; Correct: the valid numbers pass, the error check refuses 5: "Illegal
; Box# For Page".
; @fixed #82
; @configs clib
; @pass runtime_errors=on
; @pass timeout=30
BitMap 0,320,200,2
InitPage 0,0,2
AddBox 0,0,0,0,80,12,2,"A"
AddBox 0,1,0,20,80,12,2,"B"
NPrint "before"
BLITZ
BitMapOutput 0
SelectBox 1
SelectBox -1
AMIGA
DefaultOutput
NPrint "valid passed"
BLITZ
BitMapOutput 0
SelectBox 5
AMIGA
DefaultOutput
NPrint "after"
End
LoadShape 0,"x"
