; rimisclib-ezrequestargs-stale-format (#747) - EZRequestArgs builds the
; format text of its requester (%ld per long argument, separated by "|",
; i.e. a line feed) in a buffer of 160 bytes and writes no 0 behind it: a
; call with fewer arguments than the one before keeps the end of the old
; text, the requester gets more formats than arguments (the values of the
; extra ones are stale ones of the argument array).  The program calls
; EZRequestArgs with three longs, then with one; the requesters show the
; format text of each call.
; Correct: the second requester's text is "%ld" alone.
DEFTYPE .l

!TestCase{two_calls}
  Screen 0,0,0,320,200,2,0,"Workbench",1,2
  a=1 : b=2 : c=3 : d=42
  r=EZRequestArgs(a,b,c)
  r=EZRequestArgs(d)
  !Check{1,"done"}
!EndTestCase
