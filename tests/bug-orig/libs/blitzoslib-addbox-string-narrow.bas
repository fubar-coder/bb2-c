; blitzoslib-addbox-string-narrow (#84) - AddBox with a string box (type
; argument 4, box type 2) gets a text of w/8 bytes, filled with spaces and
; a 0 (DBRA over tlen-2).  For a box 8..15 pixels wide the text has 1
; byte: the counter is -1 and the loop writes 65536 spaces behind the
; 1-byte block.
; The program runs through inject with guard (runwrap=): every AllocMem
; of the program gets a pattern behind it, checked at FreeMem and at the
; end.
; The command after End is never run: it makes blitz2 allocate the shape
; objects (#335).
; Correct: the 1-byte text is just the 0, nothing is written behind an
; allocation.
; @fixed #84
; @configs clib
; @pass runwrap=@INJECT@|guard
; @pass timeout=30
BitMap 0,320,200,2
InitPage 0,0,1
AddBox 0,0,0,0,12,12,4
NPrint "added"
End
LoadShape 0,"x"
