; rimisclib-ezrequestargs-stale-format (#747) - EZRequestArgs builds the
; format text of its requester (%ld per long argument, separated by "|",
; i.e. a line feed) in a buffer of 160 bytes and writes no 0 behind it: a
; call with fewer arguments than the one before keeps the end of the old
; text, the requester gets more formats than arguments (the values of the
; extra ones are stale ones of the argument array).  The program calls
; EZRequestArgs with three longs, then with one; the log shows the
; format text of each requester.
; Correct: the second requester's text is "%ld" alone.
; @fixed #747
; @rwe rimisclib
; @pass timeout=30
DEFTYPE .l
Screen 0,0,0,320,200,2,0,"Workbench",1,2
a=1 : b=2 : c=3 : d=42
r=EZRequestArgs(a,b,c)
r=EZRequestArgs(d)
NPrint "done"
End
