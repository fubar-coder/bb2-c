; blitzoslib-initpage-numboxes (#1043) - InitPage computes the size of
; the box table as NumBoxes x 32 of the word sign-extended (EXT.l): for
; NumBoxes of 32768 or more the size is negative, a huge unsigned number,
; and the allocation fails (or behaves unexpectedly).  InitPage has no
; error check.
; Here runtime error checking is on: InitPage 0,0,32768, then the memory
; size of the page (+0 of the page object).
; The command after End is never run: it makes blitz2 allocate the shape
; objects (#335).
; Correct: an error check refuses the number: "Illegal Number of Boxes",
; the command is not done.
; @fixed #1043
; @configs clib
; @pass runtime_errors=on
; @pass timeout=30
DEFTYPE .l
BitMap 0,320,200,2
NPrint "before"
InitPage 0,0,32768
NPrint "after, size ",Hex$(Peek.l(Addr Page(0)))
End
LoadShape 0,"x"
