; risortlib-priority-odd-read (#825) - for every item the others are
; compared with, dosort reads the priority long at data + priority offset
; before it tests whether priorities are enabled; with the default offset
; -1 that is data - 1, an odd address: on a 68000 an address error (the
; session ends in a timeout).  The machine is the A600 (68000).
; Correct: StringSort without SortPriorityOffset sorts the list ("a b").
DEFTYPE .l
NEWTYPE .item
  nm$
End NEWTYPE

!TestCase{odd_read}
  Dim List a.item(5)
  If AddLast(a()) Then a()\nm="b"
  If AddLast(a()) Then a()\nm="a"
  r.l=StringSort(a(),SizeOf .item)
  ResetList a()
  s$=""
  While NextItem(a()) : s$+a()\nm+" " : Wend
  !CheckStrEq{s$,"a b ","sorted"}
!EndTestCase
