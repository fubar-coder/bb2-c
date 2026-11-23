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
DEFTYPE .l

!TestCase{kickstart_13}
  r1=EZRequest("Title","line 1|line 2","Yes|No")
  r2=EZRequest("body","Gadget")
  r3=EZRequest("body alone")
  v=7
  r4=EZRequest("T","value %ld|%s","%s|No",v,"text","Go")
  r5=EZRequestArgs(42,"abc")
  r6=EZRequest("three","A|B|C")
  !CheckEq{r1,0,"two_lines"}
  !CheckEq{r2,0,"one_gadget"}
  !CheckEq{r3,0,"body_alone"}
  !CheckEq{r4,0,"arguments"}
  !CheckEq{r5,0,"args"}
  !CheckEq{r6,0,"three_gadgets"}
!EndTestCase
