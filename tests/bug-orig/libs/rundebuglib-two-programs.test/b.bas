; a child of rundebuglib-two-programs, started in the foreground: it waits
; two seconds ("a" ends meanwhile) and then reads an array out of range
DEFTYPE .l
Dim a(5)
NPrint "b started"
Delay_ 100
NPrint "b still running"
n=10
a(n)=1
NPrint "b no error"
End
