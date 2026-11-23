; rimisclib-v36-ezrequest-gadgets (#1150) - EZRequest calls
; EasyRequestArgs (intuition V36) also on Kickstart 1.3 (intuition V34),
; behind intuition's jump table: the machine crashes (with runtime error
; checking Blitz's debugger reports it).  Below Kickstart 2.0 the library
; shows its requester with AutoRequest, which has at most two gadgets;
; more are an error of the program on such a machine.  Here with runtime
; error checking, three gadgets, on Kickstart 1.3.
; Correct: the error check refuses them: "More than two EZRequest gadgets
; need Kickstart 2.0".
DEFTYPE .l

!TestCase{three_gadgets}
  !ExpectError{"More than two EZRequest gadgets need Kickstart 2.0","refused"}
  r=EZRequest("body","A|B|C")
  !Check{0,"after"}
!EndTestCase
