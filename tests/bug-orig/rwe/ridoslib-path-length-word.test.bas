; ridoslib-path-length-word (#628) - GetFileName and GetPathName read
; the length of their string argument (a long) as a word, its low word.
; For an argument of 65536 characters or more they give the wrong amount
; of the string buffer back, and GetPathName searches only the first
; (length AND $FFFF) characters for its separator.  The path here is
; "a/", 198 "b", "/" and 65435 "c": 65636 characters, low word 100, so
; the original finds the "/" at position 2 instead of the one at 201.
; GetFileName gets 656 times 99 "c" and "/", then "xy" (65602
; characters, low word 66; no part reaches the original's 256 byte
; buffer, #627): the original gives back 67 bytes instead of 65603 and
; writes "xy" there, the program finds "cc" where the result should be.  The
; string buffer is raised to 300000 bytes (jobline strbuf=).
; Correct: the path part is 201 characters long and ends in "/", the
; file part is "xy".
DEFTYPE .l

!TestCase{long_arguments}
  a$="a/"+String$("b",198)+"/"+String$("c",65435)
  !Require{Len(a$)=65636,"path_argument"}
  b$=GetPathName(a$)
  !CheckEq{Len(b$),201,"path_length"}
  !CheckStrEq{Right$(b$,1),"/","path_end"}
  c$=""
  For i=1 To 656
    c$=c$+String$("c",99)+"/"
  Next
  c$=c$+"xy"
  !Require{Len(c$)=65602,"file_argument"}
  f$=GetFileName(c$)
  !CheckEq{Len(f$),2,"file_length"}
  !CheckStrEq{Left$(f$,8),"xy","file_part"}
!EndTestCase
