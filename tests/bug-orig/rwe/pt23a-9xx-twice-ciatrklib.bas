; pt23a-9xx-twice-ciatrklib (#473) - for a note with 9xx mt_SetRegs runs
; mt_CheckMoreEfx (mt_SampleOffset: start + 2 x offset, length - offset)
; and then mt_SetPeriod, which writes start and length to Paula and ends
; in mt_CheckMoreEfx again: the voice keeps the offset twice (a retrig,
; a note delay and a following note without a sample number start at
; the doubled offset).  Tested with ciatrklib (pt23a-9xx-twice:
; ritrackerlib).  The module (generated, loaded with LoadTrackerModule)
; plays sample 31 of 2000 words with 902 (256 words) in row 0.  The
; program finds the play routine's data in its own code (the library is
; linked behind it: the period table, with the four voices of 44 bytes
; behind it) and reads the start (+4) and the length (+8) of voice 0
; after row 0 was played.  The ticks are played by the program
; (ChangeTrackerVBlank, PlayTracker: speed 6, row 1 at the 7th tick; the
; CIA interrupt does not play in the emulator).
; Correct: the offset once - start + 512 bytes, length 2000 - 256.
; @fixed #473
; @rwe ciatrklib
; @pass file=pt.mod|@TESTDATA@/pt23a-9xx-twice-ciatrklib.mod
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
NPrint "start +",Peek.l(v+4)-s," length ",Peek.w(v+8)
FreeTrackerModule 0
End
