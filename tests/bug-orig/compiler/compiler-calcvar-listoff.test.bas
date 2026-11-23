; compiler-calcvar-listoff (#937) - calcvar keeps the element size
; addition of a List array (.listoff, 8 bytes for the item header) in one
; static word.  It is set before the index is evaluated; an element of a
; normal array inside the index runs calcvar again and sets it to 0, so
; a(b(1)) of a List array multiplies the index by the bare element size;
; the other way round, c(a(1)) of a normal array c, the element of the
; List array a sets it to 8 and c's index is multiplied by 4+8.
; Correct: a(b(1)) is the same element as a(3), 300; c(a(1)/100) is
; c(1), 7.
!TestCase{listoff}
  Dim List a.l(10)
  Dim b.l(5)
  Dim c.l(5)
  b(1)=3
  For i.l=0 To 10
    a(i)=i*100
  Next
  For i.l=0 To 5
    c(i)=i*7
  Next
  x.l=a(3)
  !CheckEq{x,300,"direct"}
  x=a(b(1))
  !CheckEq{x,300,"list_index_in_array"}
  x=c(1)
  !CheckEq{x,7,"plain"}
  x=c(a(1)/100)
  !CheckEq{x,7,"array_index_in_list"}
!EndTestCase
