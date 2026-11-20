; riseriallib-readstring-open (#796) - SerialReadString with the port open
; pushes d0 and returns to it (the long it pushed, not its return
; address): the program goes on at whatever d0 held at the call (the
; session ends in a timeout).  The program sends "OK" and a 0 to the
; remote end (serial=on, tests/corpus/runner/serial.h), which echoes them
; (request E), and reads them with SerialReadString.
; Correct: SerialReadString returns the string up to the 0: "OK".
; @rwe riseriallib
; @pass serial=on
; @pass timeout=30
DEFTYPE .l
SerialOpen 19200
q$=Chr$(1)+"riseriallib-readstring-open"+Chr$(2)+"OK"
For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
SerialWrite 0 : SerialWrite 5 : SerialWrite Asc("E")
a$=SerialReadString
NPrint "string: ",a$
SerialWrite 4 : VWait 10
End
