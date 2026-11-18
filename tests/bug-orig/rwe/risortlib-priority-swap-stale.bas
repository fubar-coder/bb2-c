; risortlib-priority-swap-stale (#826) - StringSort with SortPriorityOffset
; is a selection sort: the item in front is compared with every following
; one.  When a following item has the larger priority, the original swaps
; the data without loading that item's string and without taking over its
; priority: every later item with a priority above the first item's old
; one is swapped in as well, so the last larger one ends in front, not the
; largest.  Here the priorities decide every comparison (no two are
; equal), so no string comparison is reached and the result does not
; depend on the stale string.
; Correct: sorted by the priority, the larger first: "3 b", "2 c", "1 a".
; @fixed #826
; @rwe risortlib
NEWTYPE .item
  pri.l
  nm$
End NEWTYPE
Dim List a.item(5)
If AddLast(a()) Then a()\pri=1,"a"
If AddLast(a()) Then a()\pri=3,"b"
If AddLast(a()) Then a()\pri=2,"c"
SortPriorityOffset 0
r.l=StringSort(a(),SizeOf .item,4)
ResetList a()
While NextItem(a()) : NPrint a()\pri," ",a()\nm : Wend
End
