; ridoslib-failed-startnotify-kept (#621) - AddNotify stores the request
; (48 bytes) and the name copy in its tables before StartNotify; when
; StartNotify fails it returns 0 but leaves both there, uncounted.  finit
; ends the requests from id 63 down only while the counter is not 0, so
; it stops after the counted request 40 and never frees request 2.  The
; program starts request 2 on a missing volume (the process's requesters
; off: pr_WindowPtr -1, put back after) and request 40 on a file in RAM:,
; then ends; inject's track reports what is still allocated at the end.
; Correct: the failed request is freed at once (AddNotify 0), the started
; one at the end: no allocation left.
; @fixed #621
; @rwe ridoslib
; @pass runwrap=@INJECT@|track
DEFTYPE .l
p=Peek.l(Peek.l(4)+276)
w=Peek.l(p+184)
Poke.l p+184,-1
a=AddNotify(2,"RWENOSUCHDEVICE:x")
Poke.l p+184,w
b=AddNotify(40,"RAM:ridoslib-failed-startnotify")
NPrint "results ",a," ",b
End
