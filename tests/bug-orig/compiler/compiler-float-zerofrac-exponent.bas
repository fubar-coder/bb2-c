; compiler-float-zerofrac-exponent (#11) - with a fraction of value 0
; fetchpoint2 skips the test for an exponent: "1.0e5" stays 1.0 and "e5" is
; left unread.
; Correct: the exponent counts as it does for "1e5" and "1.5e5".
a.f=1.0e5
NPrint a
b.f=2.00E3
NPrint b
End
