; ridisplaylib-initview-leak (#891) - InitView keeps only the last copper
; list: every call allocates a new list (exec's AllocMem, chip memory)
; and forgets the one before; finit frees only the last.  The program
; makes ten lists and ends; it runs through inject (runwrap=, inject
; track: what the process leaves allocated).  The height equals the
; colour count, so a list is 502 bytes with the height or with the colours
; in its size (#890).
; Correct: nothing is left allocated.
DEFTYPE .l

!TestCase{ten_lists}
  k=0
  For i=1 To 10
    c=InitView(32,5,8,32)
    If c=0 Then k+1
  Next
  !CheckEq{k,0,"lists_failed"}
!EndTestCase
