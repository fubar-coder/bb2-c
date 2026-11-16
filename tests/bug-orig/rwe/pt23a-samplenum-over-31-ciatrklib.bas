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
; CIA interrupt does not play in the emulator).
; Correct: a sample number above 31 sets no new sample - the start stays
; the one of sample 1.
; @fixed #480
; @rwe ciatrklib
; @pass file=pt.mod|@TESTDATA@/pt23a-samplenum-over-31-ciatrklib.mod
DEFTYPE .l
Function.l findpt{b.l}
  For a.l=b To b+131072 Step 2
    If Peek.l(a)=856*65536+808
      If Peek.l(a+4)=762*65536+720 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function
here:
r=LoadTrackerModule(0,"CORPUS:pt.mod")
If r=0 Then NPrint "no module" : End
m=GetTrackerLocation(0)
s=m+$43C+1024
ChangeTrackerVBlank
r=StartTracker(0)
For i=1 To 13 : PlayTracker : Next
StopTracker
t=findpt{?here}
NPrint "voices found ",t<>0
v=t+1152
p=Peek.l(v+4)
NPrint "sample 1 ",p=s," module ",p=m
FreeTrackerModule 0
End
