; riamosfunclib-saveexec-skipped-hunk (#502) - SaveExec writes only the
; HUNK_CODE items of the list but counts every item: a code hunk behind a
; HUNK_SYMBOL item gets a HUNK_RELOC32 that names hunk 1, and the header
; a table size of 2, although the file has only hunk 0.  LoadSeg refuses
; the file.
; Correct: the file loads (LoadSeg gives a segment).
; @fixed #502
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
Dim c.l(1),rl.l(0)
c(0)=0 : c(1)=$4E754E75
rl(0)=0
If AddItem(h())
  h()\hunktype=$3F0 : h()\hunkloc=&c(0) : h()\hunklen=8
EndIf
If AddItem(h())
  h()\hunktype=$3E9 : h()\hunkloc=&c(0) : h()\hunklen=8
  h()\reloc=&rl(0) : h()\reloclen=4
EndIf
r=SaveExec("RAM:riamosfunclib-skip.exe",h())
s=LoadSeg_("RAM:riamosfunclib-skip.exe")
If s Then NPrint "loaded" : UnLoadSeg_ s Else NPrint "not loaded"
End
