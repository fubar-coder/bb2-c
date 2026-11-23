; gadgetslib-specialinfo-leak (#163) - freegadget (the _free routine of a
; GadgetList, Free GadgetList and the program end) loads the gadget type
; and overwrites it with MOVEQ #22,d0 before comparing it with 3 (prop)
; and 4 (string): the SpecialInfo of string gadgets (StringInfo and
; buffer) and prop gadgets (PropInfo) is never freed.
; Here: 20 times a StringGadget with a buffer of 1000 bytes and a
; PropGadget, then Free GadgetList; the free memory (exec AvailMem) before
; and after.
; Correct: the free memory does not shrink by more than 2000 bytes.  The
; original keeps about 1080 bytes per round.
DEFTYPE .l

!TestCase{free_special_info}
  MOVE.l 4,a6 : MOVEQ #0,d1 : JSR -216(a6) : PutReg d0,m1
  For i=1 To 20
    StringGadget 0,10,10,0,1,1000,100
    PropGadget 0,10,30,0,2,100,10
    Free GadgetList 0
  Next
  MOVE.l 4,a6 : MOVEQ #0,d1 : JSR -216(a6) : PutReg d0,m2
  !Check{m1-m2<2000,"lost_under_2000"}
!EndTestCase
