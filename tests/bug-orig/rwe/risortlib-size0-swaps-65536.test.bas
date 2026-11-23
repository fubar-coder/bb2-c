; risortlib-size0-swaps-65536 (#827) - StringSort swaps size - 1 + 1
; bytes with a DBRA counter: a size of 0 swaps 65536 bytes from the data
; of the two items on.  Made safe: the list has 1001 items of 172 bytes
; (172 KB), the two items in use are its first ones, so the 64 KB stay
; inside the list's own memory (its free items); a marker long 40000
; bytes behind the first item shows the overwrite.
; Correct: a size of 0 swaps nothing, the marker stays, and StringSort
; returns 0.
DEFTYPE .l
NEWTYPE .item
  nm$
  d.l[40]
End NEWTYPE

!TestCase{size0}
  Dim List a.item(1000)
  If AddLast(a()) Then a()\nm="b"
  If AddLast(a()) Then a()\nm="a"
  f.l=Peek.l(ListBase(a())-28)
  m.l=f+40000
  Poke.l m,$12345678
  r.l=StringSort(a(),0)
  k=(Peek.l(m)=$12345678)
  !CheckEq{k,-1,"marker_kept"}
  !CheckEq{r,0,"result"}
!EndTestCase
