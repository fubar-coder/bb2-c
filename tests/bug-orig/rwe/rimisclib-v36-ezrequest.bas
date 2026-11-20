; rimisclib-v36-ezrequest (#1150) - EZRequest and EZRequestArgs call
; EasyRequestArgs (intuition V36) with intuition.library opened in any
; version: on Kickstart 1.3 (intuition V34) the call lies behind
; intuition's jump table, the machine crashes at the first requester (the
; session ends in a timeout).  The program shows each form once on
; Kickstart 1.3 (the session answers every AutoRequest with its right
; gadget): two lines and two gadgets with a title, one gadget, the body
; alone, arguments in the body and in the gadgets, EZRequestArgs, and
; three gadgets.
; Correct: below Kickstart 2.0 the library shows an AutoRequest: the body
; formatted with the arguments, '|' and line feeds as line breaks, the
; left of two gadgets positive, the right (or only) one negative, no
; title; every result 0 (the right gadget); three gadgets: no requester,
; 0.
; @fixed #1150
; @rwe rimisclib
; @machine a500
; @pass timeout=30
DEFTYPE .l
NPrint "before"
NPrint EZRequest("Title","line 1|line 2","Yes|No")
NPrint EZRequest("body","Gadget")
NPrint EZRequest("body alone")
v=7
NPrint EZRequest("T","value %ld|%s","%s|No",v,"text","Go")
NPrint EZRequestArgs(42,"abc")
NPrint EZRequest("three","A|B|C")
NPrint "after"
End
