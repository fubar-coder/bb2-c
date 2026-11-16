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
; @fixed #1071
; @configs clib
; @pass runwrap=@INJECT@|allocfail=100000-200000/1 track
DEFTYPE .l
e=Peek.l(4)
n=0
SetErr
  n=n+1
End SetErr
If n=0
  NPrint "start"
  Dim a.l(30000)
  NPrint "no error"
  End
EndIf
NPrint "caught"
Dim b.l(10)
c=Peek.l(4)
If c=e
  NPrint "execbase kept"
Else
  Poke.l 4,e
  NPrint "execbase overwritten"
EndIf
b(3)=7
NPrint "b ",b(3)
End
