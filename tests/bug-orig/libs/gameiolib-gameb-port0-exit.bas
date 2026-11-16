; gameiolib-gameb-port0-exit (#415) - Gameb(Port) reads a CD32 pad: it
; drives pin 5 of the port low (POTGO OUTxX, shift mode), clocks the
; buttons out with the fire line and reads them from pin 9 (DATxY).  At
; the end it drives pin 5 high again: POTGO $3000 (OUTRX/DATRX) for port
; 1, $0300 (OUTLX/DATLX) for port 0.  The loop of port 0 ends with
; DBF d1,'loop, the label of the port 1 loop, and so leaves through the
; port 1 exit: after Gameb(0) POTGO is $3000, pin 5 of port 0 is an
; input again (still low from the shift mode) and pin 5 of port 1 an
; output.  POTGOR ($DFF016) is read right after the call: bit 8 (DATLX)
; is pin 5 of port 0, bit 12 (DATRX) pin 5 of port 1.  Interrupts are off
; meanwhile (input.device must not see the fire line).
; Correct: after Gameb(0) pin 5 of port 0 is high (bit 8 = 1), as pin 5
; of port 1 is after Gameb(1).
; @fixed #415
DEFTYPE .l
MOVE.l 4,a6 : JSR -120(a6)
g=Gameb(1)
a=Peek.w($DFF016) AND $FFFF
g=Gameb(0)
b=Peek.w($DFF016) AND $FFFF
MOVE.l 4,a6 : JSR -126(a6)
NPrint "after Gameb(1): port 1 pin 5 ",(a LSR 12) AND 1
NPrint "after Gameb(0): port 0 pin 5 ",(b LSR 8) AND 1
End
