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
DEFTYPE .l

!TestCase{empty_field}
  a=NumDays("/5/1990")
  b=NumDays("1/5/1990")
  DateFormat 1
  c=NumDays("5//1990")
  d=NumDays("2/1//1978")
  e=NumDays("2/1/1978")
  DateFormat 0
  !CheckEq{a,0,"day"}
  !CheckEq{b,4503,"day_filled"}
  !CheckEq{c,0,"format_1_day"}
  !CheckEq{d,0,"format_1_year"}
  !CheckEq{e,31,"format_1_filled"}
!EndTestCase
