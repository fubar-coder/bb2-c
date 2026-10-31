; blitzcoplib-maxslice-zero (#406) - after every insertion into the
; copper list inscop2 moves the pointers of all slice objects in a loop
; MOVE maxslice,d1:SUBQ #1,d1 ... DBF d1: with a Slice maximum of 0 (the
; compiler options) it visits 65536 objects of 64 bytes, i.e. 4 MB from
; the empty object area on, and "relocates" five longs of every step
; whose first word is not 0.
; Here: Slice maximum 0 in the default options (tedstub max=, Slice is
; library 144), one CustomCop (needs no slice) in Amiga mode.  The
; original does not come back (memory corrupted: the session times out).
; Correct: with no slice objects no pointer is moved, the program goes on.
; @configs clib
; @pass jobline=max=144|0
; @pass timeout=30
DEFTYPE .l
NPrint "before"
CustomCop Mki$($180)+Mki$($123),100
NPrint "after"
End
