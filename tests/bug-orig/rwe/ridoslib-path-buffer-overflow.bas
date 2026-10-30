; ridoslib-path-buffer-overflow (#627) - GetFileName builds its result in
; tbuf (256 bytes, in the code hunk) without a limit: a longer file part
; runs over the code behind it, first NotifyPort (MOVE.l msgport,d0 :
; RTS).  The name is 300 characters "NuNu...": behind tbuf they are the
; words $4E75 (RTS), so NotifyPort returns at once with whatever d0
; holds.  (The C build overwrites other data of the library instead.)
; Correct: tbuf is not overrun, NotifyPort returns the same port as
; before.
; @rwe ridoslib
DEFTYPE .l
p=NotifyPort
a$=String$("Nu",150)
b$=GetFileName(a$)
NPrint "length ",Len(b$)
If NotifyPort=p Then NPrint "port as before" Else NPrint "port changed"
End
