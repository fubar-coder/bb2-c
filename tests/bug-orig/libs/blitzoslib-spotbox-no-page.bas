; blitzoslib-spotbox-no-page (#83) - SpotBox has no page check (the other
; Blitz mode commands raise "UnInitialised Page" for a page whose memory
; size is 0: no InitPage, FreePage, or 0 boxes) and counts the boxes down
; with DBRA from numboxes-1: with 0 boxes it scans 65536 entries behind
; the page's memory.
; Here runtime error checking is on and the page has 0 boxes.
; The command after End is never run: it makes blitz2 allocate the shape
; objects (#335).
; Correct: the error check refuses the page: "UnInitialised Page".
; @fixed #83
; @configs clib
; @pass runtime_errors=on
; @pass timeout=30
BitMap 0,320,200,2
InitPage 0,0,0
NPrint "before"
BLITZ
n=SpotBox
AMIGA
NPrint "spot ",n
End
LoadShape 0,"x"
