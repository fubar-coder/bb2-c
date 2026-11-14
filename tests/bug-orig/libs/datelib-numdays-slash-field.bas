; datelib-numdays-slash-field (#123) - NumDays reads a field of the date
; up to the next "/"; a "/" as the first character of a field ends the
; field at once with the "/" itself, 47, as its value (in the year field
; it jumps back and makes 47 the second field).  So "/5/1990" is the
; 47th of May 1990.
; Here dates with an empty field: the day of dd/mm/yyyy, the day of
; mm/dd/yyyy (DateFormat 1), and an empty year of mm/dd/yyyy (the day
; becomes 47 there); the same dates with the field filled for reference.
; Correct: a date with an empty field is invalid input, NumDays returns 0
; as for its other invalid input.  The original counts 47 for the field.
; @fixed #123
; @configs clib
DEFTYPE .l
NPrint "[/5/1990] ",NumDays("/5/1990")
NPrint "[1/5/1990] ",NumDays("1/5/1990")
DateFormat 1
NPrint "fmt1 [5//1990] ",NumDays("5//1990")
NPrint "fmt1 [2/1//1978] ",NumDays("2/1//1978")
NPrint "fmt1 [2/1/1978] ",NumDays("2/1/1978")
End
