; riamosfunclib-devicename-empty (#493) - DeviceName$ cuts a ":" at the
; end of its argument by testing the byte before the argument's 0
; (-2(a3)), also for an empty argument: then that byte belongs to what
; the string expression built before, and a ":" there is cleared.
; "X:"+DeviceName$("") loses its ":".
; Correct: b$ is "X:" (DeviceName$("") finds nothing: ""): the length 2,
; the second character 58 (":"; the original clears it to 0).
DEFTYPE .l

!TestCase{devicename_empty}
  b$="X:"+DeviceName$("")
  !CheckEq{Len(b$),2,"length"}
  !CheckEq{Asc(Mid$(b$,2,1)),58,"colon"}
!EndTestCase
