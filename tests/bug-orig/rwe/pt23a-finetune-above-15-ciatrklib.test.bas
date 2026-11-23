; pt23a-finetune-above-15-ciatrklib (#476) - the finetune byte of a
; sample header is taken unmasked; every period lookup multiplies it by
; 72 bytes: a finetune of 16 reads behind the period table (in both
; builds the four voices follow it).  Tested with ciatrklib
; (pt23a-finetune-above-15: ritrackerlib).  The module (generated,
; loaded with LoadTrackerModule) plays sample 31 with finetune 16 at
; period 428 in row 0.  The program finds the play routine's data in its
; own code (the library is linked behind it: the period table, with the
; four voices of 44 bytes behind it) and reads the period (+16) of voice
; 0 after row 0 was played.  The ticks are played by the program
; (ChangeTrackerVBlank, PlayTracker: speed 6, row 1 at the 7th tick; the
; program counts the ticks itself instead of the CIA interrupt).
; Correct: the finetune is masked to 0..15 (16 -> 0): period 428.
DEFTYPE .l
Function.l findpt{b.l}
  For a.l=b To b+131072 Step 2
    If Peek.l(a)=856*65536+808
      If Peek.l(a+4)=762*65536+720 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function

!TestCase{finetune}
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
  !CheckEq{Peek.w(v+16),428,"period"}
  FreeTrackerModule 0
!EndTestCase
