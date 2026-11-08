; compiler-newtype-forward-field (#974) - a pointer field that names a type
; not defined yet (*p.fwd) makes typemode add a forward type node for .fwd
; with the line number of that field as its size.  A later field of that
; type by value (x.fwd in .dd) takes the line number as its size; the
; NEWTYPE .fwd defined afterwards does not correct the layout of .dd: the
; field y and SizeOf .dd depend on the line of *p.fwd (here 10).
; Correct: the layout of .dd follows the size of .fwd (8 bytes): y at 8,
; SizeOf .dd 12, and the fields of v do not overlap.
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
  y.l
End NEWTYPE
NEWTYPE .fwd
  a.l
  b.l
End NEWTYPE
DEFTYPE .dd v
v\x\b=7
v\y=9
o.l=SizeOf .dd\y
NPrint "y at ",o
NPrint "size ",SizeOf .dd
NPrint v\x\b," ",v\y
End
