; riamosfunclib-saveexec-two-hunks (#503) - SaveExec patches the
; HUNK_HEADER with first hunk 0, last hunk 0 and one size slot (the sum of
; all code sizes) and writes one HUNK_END behind all hunks: a list with two
; code items gives a file LoadSeg cannot load (in the emulator the
; LoadSeg of it crashes: the session halts).
; Correct: the file loads (LoadSeg gives a segment).
; @fixed #503
; @rwe riamosfunclib
DEFTYPE .l
NEWTYPE .hunk
hunktype.w
hunkloc.l
hunklen.l
reloc.l
reloclen.l
flags.l
End NEWTYPE
Dim List h.hunk(4)
Dim c.l(1)
c(0)=0 : c(1)=$4E754E75
If AddItem(h())
  h()\hunktype=$3E9 : h()\hunkloc=&c(0) : h()\hunklen=8
EndIf
If AddItem(h())
  h()\hunktype=$3E9 : h()\hunkloc=&c(0) : h()\hunklen=8
EndIf
r=SaveExec("RAM:riamosfunclib-two.exe",h())
s=LoadSeg_("RAM:riamosfunclib-two.exe")
If s Then NPrint "loaded" : UnLoadSeg_ s Else NPrint "not loaded"
End
