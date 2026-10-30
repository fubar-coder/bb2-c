; riintuilib-output-buffer-overflow (#698) - the IPOutput handler copies
; a string with length (a0/d0) into its 1024-byte buffer without a
; limit; the pens apen/bpen follow the buffer.  A string of 1026
; characters is drawn whole: 1023 spaces and "xxx", the last two
; characters land in apen (the next text's pen; the original takes the
; pens before the copy, so this text is still drawn in pen 1).
; Made safe: the overflow is 3 bytes (the pens only); the bitmap is
; 320 x 256 x 8, so the unclipped long text (8208 pixels wide, it runs on
; through the rows of the plane) stays inside its planes.  Spaces draw
; nothing (JAM1); the "xxx" (characters 1023-1025) lands in bytes 23-25
; of the plane rows 25-32, x 184-207.  The program prints the highest
; pen it finds there.
; Correct: the string is cut to the buffer (1023 characters, all
; spaces): nothing is drawn, pen 0.
; @rwe riintuilib
BitMap 0,320,256,8
Cls 0
IPBitMap 0,0
IPOutput 0
Print String$(" ",1023)+"xxx"
DefaultOutput
p=0
For y=22 To 36
  For x=184 To 207
    If Point(x,y)>p Then p=Point(x,y)
  Next
Next
NPrint "pen ",p
End
