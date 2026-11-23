; riarexxlib-rexxsyslib-never-closed (#1093) - the library's init opens
; rexxsyslib.library at every program start; finit only deletes the port
; and never closes it.  The program runs through inject, which counts the
; opens minus the closes of rexxsyslib.library by the process
; (opencount=); it creates and deletes a port.
; Correct: rexxsyslib.library is closed again (+0); the original leaves
; it +1.
DEFTYPE .l

!TestCase{create_and_delete}
  p=RXCreatePort("BUGORIG")
  created=(p<>0)
  !CheckEq{created,-1,"created"}
  RXDeletePort
!EndTestCase
