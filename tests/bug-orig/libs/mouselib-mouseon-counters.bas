; mouselib-mouseon-counters (#223) - Mouse On remembers the start values
; of the mouse counters for its vertical blank interrupt, but swapped:
; MOVE.b $dff00a,lx : MOVE.b $dff00b,ly, while $DFF00A is the Y counter
; and $DFF00B the X counter (the interrupt reads them that way).  The
; first interrupt then moves the mouse by the difference of the two
; counters.  The program sets the upper six bits of the Y counters to
; 001000 and those of the X counters to 0 through JOYTEST ($DFF036), so
; the two counters of JOY0DAT differ by 32; the mouse is not moved.
; Correct: the position after Mouse On and three frames is the one set
; with PositionMouse (160 100).
; @fixed #223
DEFTYPE .l
BLITZ
MouseArea 0,0,640,512
PositionMouse 160,100
Poke.w $DFF036,$2000
j=Peek.w($DFF00A) AND $FFFF
Mouse On
VWait 3
x.q=MouseX : y.q=MouseY
Mouse Off
AMIGA
NPrint "JOY0DAT ",Hex$(j)
NPrint "mouse ",x," ",y
End
