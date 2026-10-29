; blitzoslib-backspace-cursor (#91) - BlitzEvent in input mode turns the
; cursor off (docursor inverts it) before it looks at the key; backspace
; at position 0 and control characters other than backspace and DEL
; (Return, Esc, Tab, codes > 127) leave to dunedit without turning it on
; again.  The program puts a string box on a page, selects it and starts
; input mode (ActivateSelectedBox draws the cursor at position 0), sums
; plane 0 of the bitmap, puts one backspace event (raw code $41) into
; keyboard.device's buffer (read index +$70, write index +$72, entries
; {code, qualifier} from +$74), where BlitzKey takes it, calls
; BlitzEvent and sums plane 0 again.
; The command after End is never run: it makes blitz2 allocate the shape
; objects (#335 - blitzoslib's drawing pulls in the shapes library only
; behind calcmaxs, and End then frees 100 shapes from address 0).
; The original leaves the cursor off: plane 0 differs.
; Correct: the cursor is on again, plane 0 as before.
; @configs clib
DEFTYPE .l
Function.l sum{p.l}
  t=0
  For i=0 To 7996 Step 4 : t+Peek.l(p+i) : Next
  Function Return t
End Function
n=Peek.l(Peek.l(4)+350) : k=0
While Peek.l(n)<>0
  If Peeks$(Peek.l(n+10),15)="keyboard.device" Then k=n
  n=Peek.l(n)
Wend
If k=0 Then NPrint "no keyboard.device" : End
BitMap 0,320,200,2
p=Peek.l(Addr BitMap(0)+8)
InitPage 0,0,1
AddBox 0,0,16,16,80,12,4
BLITZ
BitMapOutput 0
RedrawPage
SelectBox 0
ActivateSelectedBox
a=sum{p}
r=Peek.w(k+$70) : w=Peek.w(k+$72) : x=Peek.l(k+$78)
Poke.w k+$78,$41 : Poke.w k+$7A,0
Poke.w k+$70,0 : Poke.w k+$72,8
e=BlitzEvent
g=Peek.w(k+$70)
b=sum{p}
Poke.w k+$70,r : Poke.w k+$72,w : Poke.l k+$78,x
AMIGA
DefaultOutput
NPrint "key taken ",g=4
NPrint "event ",e
NPrint "plane 0 as before ",a=b
End
LoadShape 0,"x"
