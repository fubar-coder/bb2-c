; rawkeylib-qconvertkey-index (#245) - qconvertkey indexes its two tables
; of 105 entries (raw codes 0-104, normal and shifted) with the raw code
; & 255 without a limit: raw codes 105 and up read the shifted table
; (without shift) or the library's code behind it (with shift).
; The caller is blitzkeyslib BlitzKey, which takes the next key down
; event from keyboard.device's buffer (read index +$70, write index +$72,
; entries {code, qualifier} from +$74).  No key can be pressed here: the
; program puts one event into that buffer itself - raw code $78 (the
; reset warning, 120) without and raw code $69 (105) with the left shift
; qualifier - calls BlitzKey and puts the buffer back as it was.
; The original returns the shifted character of raw code 15 and a byte of
; its own code.
; Correct: no character for a raw code beyond the tables (0 and 0).
; @configs clib
DEFTYPE .l
Function.l inject{c.l,q.l}
  SHARED k
  r=Peek.w(k+$70) : w=Peek.w(k+$72) : e=Peek.l(k+$78)
  Poke.w k+$78,c : Poke.w k+$7A,q
  Poke.w k+$70,0 : Poke.w k+$72,8
  v=BlitzKey
  Poke.w k+$70,r : Poke.w k+$72,w : Poke.l k+$78,e
  Function Return v
End Function
n=Peek.l(Peek.l(4)+350) : k=0
While Peek.l(n)<>0
  If Peeks$(Peek.l(n+10),15)="keyboard.device" Then k=n
  n=Peek.l(n)
Wend
If k=0 Then NPrint "no keyboard.device" : End
NPrint "raw $78 ",inject{$78,0}
NPrint "raw $69 shift ",inject{$69,1}
End
