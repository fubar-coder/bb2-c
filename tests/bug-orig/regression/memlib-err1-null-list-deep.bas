; memlib-err1-null-list-deep - the fix of #1071 (bug-orig test
; memlib-err1-null-list) with the "Out of Memory!" 12 procedure levels
; deep: memlib's err1 frees the 12 local lists (with freelast, 13 list
; headers in two buckets with the global one), the blocks of the global
; list and the bucket above the first one, and keeps the global list
; header.  The program runs through inject: the AllocMem of the local
; array l (120000 bytes and memlib's 8) fails once; track reports what
; is left at the end.  The SetErr handler goes on behind End SetErr;
; there the array b is allocated and address 4 is compared with ExecBase,
; then a local array 12 levels deep again.
; Correct: address 4 still holds ExecBase, the program goes on, and
; nothing stays allocated.  (The original halts the CPU after "start".)
; @pass runwrap=@INJECT@|allocfail=100000-200000/1 track
DEFTYPE .l
Statement deep{d}
  If d>0
    deep{d-1}
  Else
    Dim l.l(30000)
    NPrint "no error"
  EndIf
End Statement
Statement deep2{d}
  If d>0
    deep2{d-1}
  Else
    Dim m.l(10)
    m(2)=5
    NPrint "m ",m(2)
  EndIf
End Statement
e=Peek.l(4)
n=0
SetErr
  n=n+1
End SetErr
If n=0
  NPrint "start"
  deep{12}
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
deep2{12}
End
