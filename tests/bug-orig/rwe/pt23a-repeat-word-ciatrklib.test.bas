; pt23a-repeat-word-ciatrklib (#479) - the loop start of a sample is
; start + 2 x repeat, the doubling done as a word (ASL.W): from a repeat
; of 32768 words on the loop starts 64 KB too early.  Tested with
; ciatrklib (pt23a-repeat-word: ritrackerlib).  The module (generated,
; loaded with LoadTrackerModule) plays sample 31 with a repeat from word
; $8000 in row 0 (Paula then reads chip memory behind the module,
; nothing is written).  The program finds the play routine's data in its
; own code (the library is linked behind it: the period table, with the
; four voices of 44 bytes behind it) and reads the start (+4) and loop
; start (+10) of voice 0 after row 0 was played.  The ticks are played
; by the program (ChangeTrackerVBlank, PlayTracker: speed 6, row 1 at
; the 7th tick; the program counts the ticks itself instead of the CIA
; interrupt).
; Correct: the loop starts $10000 bytes behind the start.
DEFTYPE .l
Function.l findpt{b.l}
  For a.l=b To b+131072 Step 2
    If Peek.l(a)=856*65536+808
      If Peek.l(a+4)=762*65536+720 Then Function Return a
    EndIf
  Next
  Function Return 0
End Function

!TestCase{repeat_word}
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
  !CheckEq{Peek.l(v+4)-s,0,"start"}
  !CheckEq{Peek.l(v+10)-Peek.l(v+4),$10000,"loop_offset"}
  FreeTrackerModule 0
!EndTestCase
