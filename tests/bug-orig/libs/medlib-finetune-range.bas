; medlib-finetune-range (#211) - the finetune of a track (InstrExt or
; command 15xx, range -8..7, checked only with #CHECK = 1) times 4 indexes
; the 16 pointers to the period tables from _periodtable+32 without a
; check.  The module here (generated) plays note 13 with 15 08 (finetune
; 8) in line 0 of track 0: the pointer is read from behind the table -
; the pointer to the 64 zero bytes in chip memory (chipzero), so the period
; of the note is 0 (the channel stays silent).  The Paula registers cannot
; be read back: the program finds the track data of the player in its own
; code (the library is linked behind it; tracks 0-3, 98 bytes each, hold
; the address of their audio registers, $DFF0A0 + 16 * track, at +22) and
; points track 0 to a buffer, where the player then writes LC, LEN, PER
; (+6) and VOL.  PlayMed plays one tick.
; Correct: a finetune outside -8..7 is limited (or ignored) - the period
; of the note comes from one of the tables (113..3424).
; @configs clib
; @pass file=fine.med|@TESTDATA@/medlib-finetune-range.med
DEFTYPE .l
Function.l finddb{b.l}
  For a.l=b To b+65536 Step 2
    If Peek.l(a)=$DFF0A0
      If Peek.l(a+98)=$DFF0B0
        If Peek.l(a+196)=$DFF0C0 Then Function Return a
      EndIf
    EndIf
  Next
  Function Return 0
End Function
here:
Dim r.l(16)
x=finddb{?here}
NPrint "tracks found ",x<>0
If x=0 Then End
Poke.l x,&r(0)
LoadMedModule 0,"CORPUS:fine.med"
StartMedModule 0
PlayMed
p=Peek.w(&r(0)+6) AND $FFFF
NPrint "period from a table ",p>=113 AND p<=3424
StopMed
Poke.l x,$DFF0A0
End
