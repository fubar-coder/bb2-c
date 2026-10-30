; compiler-calcvar-listoff (#937) - calcvar keeps the element size
; addition of a List array (.listoff, 8 bytes for the item header) in one
; static word.  It is set before the index is evaluated; an element of a
; normal array inside the index runs calcvar again and sets it to 0, so
; a(b(1)) of a List array multiplies the index by the bare element size.
; Correct: a(b(1)) is the same element as a(3), the program prints 300
; twice.
Dim List a.l(10)
Dim b.l(5)
b(1)=3
For i.l=0 To 10
  a(i)=i*100
Next
NPrint a(3)
NPrint a(b(1))
End
