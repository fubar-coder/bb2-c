; datelib-numdays-month-range (#124) - NumDays indexes its table of month
; starts with the month without a check: month 0 reads the word before
; the table (the low word of the ticks of the last DateStamp, 0 before
; SystemDate or Hours), month 13 the table's end mark 1000, months 14 and
; 15 the two words behind it (the code at the end of the library), larger
; months whatever memory follows.
; Here NumDays with the months 0, 13, 14 and 15 (no SystemDate or Hours
; before), and with 12 for reference.
; Correct: a month outside 1-12 is invalid input, NumDays returns 0 as for
; its other invalid input.  The original gives a count from the words it
; reads.
; @fixed #124
; @configs clib
DEFTYPE .l
NPrint "[1/0/1990] ",NumDays("1/0/1990")
NPrint "[1/12/1990] ",NumDays("1/12/1990")
NPrint "[1/13/1990] ",NumDays("1/13/1990")
NPrint "[1/14/1990] ",NumDays("1/14/1990")
NPrint "[1/15/1990] ",NumDays("1/15/1990")
DateFormat 1
NPrint "fmt1 [13/1/1999] ",NumDays("13/1/1999")
End
