; a child of model-run-program, compiled with runtime error checking: an
; array subscript out of range shows the runtime error requester
NPrint "req"
Dim a(5)
n=10
a(n)=1
NPrint "no error"
End
