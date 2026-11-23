; blitzoslib-addbox-old-text (#89) - AddBox frees the old text of a box
; and clears its length (tlen, +20) but leaves the text pointer (+24): a
; box added again without a text keeps the pointer to the freed text.
; Drawing it (oneline) passes length and pointer swapped when the length
; is 0: the output starts at address 0 with the pointer value as its
; length.  The page object is at Addr Page(0), its +4 is the address of
; the boxes.
; The statement neverrun is never called: its LoadShape makes blitz2
; allocate the shape objects (#335).
; Correct: the box added again has no text (length and pointer 0).
DEFTYPE .l

Statement neverrun{}
  LoadShape 0,"x"
End Statement

!TestCase{old_text}
  BitMap 0,320,200,2
  InitPage 0,0,1
  b=Peek.l(Addr Page(0)+4)
  AddBox 0,0,10,10,80,12,2,"OK"
  AddBox 0,0,10,10,80,12,2
  !CheckEq{Peek.l(b+20),0,"text_length"}
  !CheckEq{Peek.l(b+24),0,"text_pointer"}
!EndTestCase
