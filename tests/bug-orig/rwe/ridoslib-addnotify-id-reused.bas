; ridoslib-addnotify-id-reused (#620) - AddNotify stores the new request
; in its table without looking at the entry: a request already there for
; that id stays active but can no longer be ended.  Id 1 watches
; RAM:bug-orig-620-a, then RAM:bug-orig-620-b; RemNotify 1 ends only the
; second.  Then RAM:bug-orig-620-a is written: the lost request still
; sends its message (id 1).
; Correct: after RemNotify 1 no request is left, no message arrives (0).
; @rwe ridoslib
DEFTYPE .l
r=AddNotify(1,"RAM:bug-orig-620-a")
r=AddNotify(1,"RAM:bug-orig-620-b")
RemNotify 1
f=Open_("RAM:bug-orig-620-a",1006)
If f Then Close_ f
Delay_ 25
NPrint CheckNotify
End
