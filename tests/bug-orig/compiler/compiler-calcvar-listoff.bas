; compiler-calcvar-listoff (#937) - calcvar keeps the element size
; addition of a List array (.listoff, 8 bytes for the item header) in one
; static word.  It is set before the index is evaluated; an element of a
; normal array inside the index runs calcvar again and sets it to 0, so
; a(b(1)) of a List array multiplies the index by the bare element size;
; the other way round, c(a(1)) of a normal array c, the element of the
; List array a sets it to 8 and c's index is multiplied by 4+8.
; Correct: a(b(1)) is the same element as a(3), the program prints 300
; twice; c(a(1)/100) is c(1), 7 twice.
; @fixed #937
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
NPrint a(3)
NPrint a(b(1))
NPrint c(1)
NPrint c(a(1)/100)
End
