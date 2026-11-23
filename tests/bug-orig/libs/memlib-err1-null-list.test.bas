; memlib-err1-null-list (#1071) - "Out of Memory!" (memlib err1) frees
; all memory of the program, which leaves no list header at all
; (firstdata = 0), and then raises the runtime error.  A program that
; catches it with SetErr goes on without a list: the next allocation
; links its block into the list at address 0, so it writes the block's
; address to address 4, over ExecBase.
; The program runs through inject: the AllocMem of the array a (120000
; bytes and memlib's 8) fails once; track reports what is left at the
; end.  The SetErr handler goes on behind End SetErr; there the array b
; is allocated and address 4 is compared with ExecBase, then put back if
; it was overwritten.  The same 12 procedure levels deep:
; regression/memlib-err1-null-list-deep.
; Correct: address 4 still holds ExecBase, and nothing stays allocated.
DEFTYPE .l

!TestCase{execbase_kept}
  eb=Peek.l(4)
  n=0
  SetErr
    n=n+1
  End SetErr
  If n=0
    Dim a.l(30000)
    !Check{0,"allocation_failed"}
  EndIf
  !CheckEq{n,1,"caught"}
  Dim b.l(10)
  c=Peek.l(4)
  Poke.l 4,eb
  !CheckEq{c,eb,"execbase"}
  b(3)=7
  !CheckEq{b(3),7,"b"}
!EndTestCase
