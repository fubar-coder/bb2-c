; t160_gameio - gameiolib without input devices touched (nothing is
; pressed, no CD32 pad): port 1 directions and buttons, port 0 (the
; mouse: its counters may be anything) compared with a decode of JOY0DAT
; read here, Gameb on both ports (no pad: the lines stay high, no bits),
; Joyb again after Gameb (Gameb leaves POTGO at $3000, also for port 0).
DEFTYPE .l
NPrint "port1 ",Joyx(1)," ",Joyy(1)," ",Joyr(1)," ",Joyb(1)
; port 0: Joyx/Joyy/Joyr against the counter word (retry if it moved)
ok=0 : n=0
While ok=0 AND n<20
  v=Peek.w($DFF00A) AND $FFFF
  x=Joyx(0) : y=Joyy(0) : r=Joyr(0)
  If (Peek.w($DFF00A) AND $FFFF)=v Then ok=1
  n=n+1
Wend
ex=0 : If v AND 2 Then ex=1
If v AND $200 Then ex=-1
w=v EOR (v LSR 1)
ey=0 : If w AND 1 Then ey=1
If w AND $100 Then ey=-1
Dim rt(15) : For i=0 To 15 : Read rt(i) : Next
er=rt(((ex AND 3)*2+(ey AND 3)*8)/2)
NPrint "port0 stable ",ok," x ",x=ex," y ",y=ey," r ",r=er
NPrint "buttons ",Joyb(0)," ",Joyb(1)
; Gameb drives the fire line low for a moment: interrupts off meanwhile,
; so that input.device cannot see a button press
MOVE.l 4,a6 : JSR -120(a6)
g1=Gameb(1) : g0=Gameb(0)
MOVE.l 4,a6 : JSR -126(a6)
NPrint "gameb ",g1," ",g0
NPrint "buttons after ",Joyb(0)," ",Joyb(1)
; values in Quick/Long context (word functions)
q.q=Joyr(1) : l.l=Joyx(1)
NPrint "types ",q," ",l
End
Data 8,2,8,6,4,3,8,5,8,8,8,8,0,1,8,7
