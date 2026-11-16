; pt23a-init-pattern-scan-ciatrklib (#477) - mt_init finds the highest
; pattern number of the 128 song positions with one decrement of its
; counter per byte read, but a new maximum decrements it once more (SUBQ,
; the DBRA skipped): position 127 is read only behind a new maximum at
; 126, and then the counter 0 becomes $FFFF and the scan reads 65535
; bytes more (the patterns and what lies behind them); the bytes are
; compared signed.  The sample starts lie behind the highest pattern.
; Tested with ciatrklib (pt23a-init-pattern-scan: ritrackerlib).  The
; module (generated, loaded with LoadTrackerModule) has pattern 1 at
; position 126 and pattern 2 at position 127, and the byte $7F in its
; pattern data; the original then takes 127 as the highest pattern.
; StartTracker runs mt_init (ciatrklib's mt_init clears no sample long
; of a loaded module, #438); the program finds the play routine's data
; in its own code (the library is linked behind it: the period table,
; the four voices of 44 bytes, fake_channel of 60 bytes and
; mt_SampleStarts behind it) and reads the start of sample 1
; (mt_SampleStarts+0) relative to the module.
; Correct: the highest pattern is 2 - sample 1 starts at 1084 + 3 x 1024
; ($103C).
; @fixed #477
; @rwe ciatrklib
; @pass file=pt.mod|@TESTDATA@/pt23a-init-pattern-scan-ciatrklib.mod
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
ChangeTrackerVBlank
r=StartTracker(0)
StopTracker
t=findpt{?here}
NPrint "voices found ",t<>0
NPrint "sample 1 start ",Hex$(Peek.l(t+1388)-m)
FreeTrackerModule 0
End
