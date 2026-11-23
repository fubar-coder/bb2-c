; pt23a-samplenum-over-31-ciatrklib (#480) - the sample number of a note
; (up to 255) indexes the table of the sample starts (32 longs, 31 set)
; and the sample headers without a check: sample 33 takes its start from
; behind the table - mt_SongDataPtr, the module's own address.  Tested
; with ciatrklib (pt23a-samplenum-over-31: ritrackerlib).  The module
; (generated, loaded with LoadTrackerModule) plays sample 1 in row 0 and
; "sample 33" in row 1.  The program finds the play routine's data in
; its own code (the library is linked behind it: the period table, with
; the four voices of 44 bytes behind it) and reads the start (+4) of
; voice 0 after row 1 was played.  The ticks are played by the program
; (ChangeTrackerVBlank, PlayTracker: speed 6, row 1 at the 7th tick; the
; program counts the ticks itself instead of the CIA interrupt).
; Correct: a sample number above 31 sets no new sample - the start stays
; the one of sample 1.
DEFTYPE .l
Function.l findpt{b.l}
  For a.l=b To b+131072 Step 2
    If Peek.l(a)=856*65536+808
      If Peek.l(a+4)=762*65536+720 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function

!TestCase{sample_number}
  here:
  dosb=OpenLibrary_("dos.library",0)       ; the original needs DOSBase in a6 here (#443)
  GetReg a6,dosb
  r=LoadTrackerModule(0,"CORPUS:pt.mod")
  !Require{r=-1,"loaded"}
  m=GetTrackerLocation(0)
  s=m+$43C+1024
  ChangeTrackerVBlank
  r=StartTracker(0)
  For i=1 To 13 : PlayTracker : Next
  StopTracker
  t=findpt{?here}
  !Require{t<>0,"voices_found"}
  v=t+1152
  p=Peek.l(v+4)
  !Check{p=s,"sample_1"}
  !Check{p<>m,"not_module"}
  FreeTrackerModule 0
!EndTestCase
