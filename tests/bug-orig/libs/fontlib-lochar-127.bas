; fontlib-lochar-127 (#148) - bitchar drops the characters below
; tf_LoChar with SUB.b tf_LoChar / BMI: a signed byte compare, so the
; characters 128 or more above tf_LoChar are dropped as well.  With the
; default BlitzFont topaz 8 (LoChar 32, HiChar 255) every code from 160
; on: the umlauts are not drawn and the cursor does not move.
; Here BitMapOutput on a bitmap of one plane, Chr$(65) ("A") and
; Chr$(196) ("A" umlaut); after each the cursor column (CursX), then the
; number of non-zero bytes of the second character cell.
; Correct: both characters are drawn, the cursor moves one column each.
; The original leaves the cursor at 1 and draws nothing for Chr$(196).
; @fixed #148
; @configs clib
DEFTYPE .l
BitMap 0,320,16,1
BitMapOutput 0
Print Chr$(65)
a=CursX
Print Chr$(196)
b=CursX
DefaultOutput
p=Peek.l(Addr BitMap(0)+8)
n=0
For i=0 To 7
  If Peek.b(p+i*40+1)<>0 Then n=n+1
Next
NPrint "after 65: ",a
NPrint "after 196: ",b
NPrint "drawn: ",n>0
End
