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
; nothing stays allocated (inject's report, asserted on `inject`).  (The
; original halts the CPU after the allocation fails.)

DEFTYPE .l

Statement deep{d}
  If d>0
    deep{d-1}
  Else
    Dim l.l(30000)
  EndIf
End Statement

Statement deep2{d,p}
  If d>0
    deep2{d-1,p}
  Else
    Dim m.l(10)
    m(2)=5
    Poke.l p,m(2)
  EndIf
End Statement

!TestCase{out_of_memory_deep}
  Dim res.l(1)
  e=Peek.l(4)
  n=0
  SetErr
    n=n+1
  End SetErr
  If n=0
    deep{12}
    !Check{0,"allocation_failed"}
    End
  EndIf
  !Check{-1,"error_caught"}
  Dim b.l(10)
  c=Peek.l(4)
  If c<>e Then Poke.l 4,e
  !CheckEq{c,e,"execbase_kept"}
  b(3)=7
  !CheckEq{b(3),7,"array_b"}
  deep2{12,&res(0)}
  !CheckEq{res(0),5,"array_m"}
!EndTestCase
