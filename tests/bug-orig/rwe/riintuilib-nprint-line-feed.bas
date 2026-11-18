; riintuilib-nprint-line-feed (#699) - the IPOutput handler's entry for
; a single character reads the character from the address d0 instead of
; taking d0: the only single character printlib sends is NPrint's line
; feed (10), so every NPrint draws the byte at address 10 instead of
; starting a new line.
; NPrint "a" : NPrint "b" (topaz 8: 8 x 8 pixels a character).  The
; program counts the pixels set in the second line (x 0-7, y 8-15).
; Correct: the line feed moves to the start of the next line, "b" is
; drawn there.
; @fixed #699
; @rwe riintuilib
BitMap 0,320,32,2
Cls 0
IPBitMap 0,0
IPOutput 0
NPrint "a"
NPrint "b"
DefaultOutput
n=0
For y=8 To 15
  For x=0 To 7
    If Point(x,y) Then n=n+1
  Next
Next
NPrint "line 2 ",n>0
End
