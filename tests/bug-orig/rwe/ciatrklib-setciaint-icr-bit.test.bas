; ciatrklib-setciaint-icr-bit (#450) - SetCIAInt (the library's init,
; StartTracker in CIA mode) loads the interrupt bit for AddICRVector with
; MOVE.l %00000001,D0 (timer A of ciaa) and MOVE.l %00000010,D0 (timer B
; of ciab): without `#` these are the longs at the ADDRESSES 1 and 2.
; With ExecBase above 16 MB (the emulator's A1200 with Zorro III memory)
; the long at 1 is $40: AddICRVector accepts that "bit", the library
; starts timer A of ciaa, but no interrupt ever calls the play routine -
; in CIA mode (the default) nothing plays.  The module (the one of the
; test of #453) has the event 805 in row 0; the program starts it in CIA
; mode and reads the event after 50 frames.
; Correct: the CIA interrupt plays the module: the event is 5.
DEFTYPE .l

!TestCase{cia_event}
  dosb=OpenLibrary_("dos.library",0)       ; the original needs DOSBase in a6 here (#443)
  GetReg a6,dosb
  r=LoadTrackerModule(0,"CORPUS:c.mod")
  !Require{r=-1,"loaded"}
  r=StartTracker(0)
  For i=1 To 50 : VWait : Next
  e=GetTrackerEvent
  StopTracker
  !CheckEq{e,5,"event"}
!EndTestCase
