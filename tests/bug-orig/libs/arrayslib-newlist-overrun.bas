; arrayslib-newlist-overrun (#59) - newlist links the items of a list
; into its free list, 8 bytes (next, last) per item, but the loop links
; the first item before any check: with a size of 0 (Dim List with the
; dimension -1, ADDQ #1 as a word gives 0 items) the list block is only
; the 32 byte header and the 8 bytes of the item land behind it.
; The program runs through inject with guard (runwrap=): every AllocMem
; of the program gets a pattern behind it, checked at FreeMem and at the
; end.  The block of the list is memlib's: 8 bytes of memlib + 32 = 40.
; Correct: nothing is written behind an allocation.  The original writes
; behind the block of 40 bytes.
; @configs clib
; @pass runwrap=@INJECT@|guard
n=-1
Dim List a.w(n)
NPrint "dimmed"
End
