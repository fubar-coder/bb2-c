; blitzoslib-blitzevent-noselect (#90) - BlitzEvent, on cursor up/down,
; tests the type of the selected box (btype, offset 8) without testing
; that a box is selected: with none selected (selected = 0, here right
; after InitPage) it reads the word at address 8.  Only if that word is
; 5 (list box) does it go on (movelist): it writes the box value to
; address 12 and draws a "box" from address 0.  The program makes that
; visible: with interrupts off (Disable) it saves addresses 0-31, puts a
; harmless list box there (40x19 at 0,0, value 1 of 3 entries, its text
; in allocated memory), presses cursor up by setting bit 4 of byte 9 of
; keyboard.device's key matrix (+$136, where RawStatus reads it), calls
; BlitzEvent, reads address 12 and puts everything back.
; The command after End is never run: it makes blitz2 allocate the shape
; objects (#335 - blitzoslib's drawing pulls in the shapes library only
; behind calcmaxs, and End then frees 100 shapes from address 0).
; The original sets address 12 to 0 (the value moved up a line).
; Correct: no box is selected, nothing is read or written: address 12
; still holds 1.
; @configs clib
DEFTYPE .l
Dim s.l(7)
n=Peek.l(Peek.l(4)+350) : k=0
While Peek.l(n)<>0
  If Peeks$(Peek.l(n+10),15)="keyboard.device" Then k=n
  n=Peek.l(n)
Wend
If k=0 Then NPrint "no keyboard.device" : End
m=AllocMem_(8,$10001)
Poke.b m,65 : Poke.b m+2,66 : Poke.b m+4,67
BitMap 0,320,200,2
InitPage 0,0,1
BLITZ
BitMapOutput 0
mk=k+$136+9 : ob.b=Peek.b(mk)
Poke.b mk,ob OR 16
r=RawStatus($4C)
Disable_
For i=0 To 7 : s(i)=Peek.l(i*4) : Next
Poke.l 0,0 : Poke.w 4,40 : Poke.w 6,19 : Poke.w 8,5 : Poke.w 10,0
Poke.w 12,1 : Poke.w 14,3 : Poke.l 16,0 : Poke.l 20,0 : Poke.l 24,m : Poke.l 28,0
e=BlitzEvent
v=Peek.w(12)
For i=0 To 7 : Poke.l i*4,s(i) : Next
Enable_
Poke.b mk,ob
AMIGA
DefaultOutput
FreeMem_ m,8
NPrint "cursor up ",r
NPrint "event ",e
NPrint "address 12 unchanged ",v=1
End
LoadShape 0,"x"
