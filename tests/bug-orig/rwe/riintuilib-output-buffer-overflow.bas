; riintuilib-output-buffer-overflow (#698) - the IPOutput handler copies
; a string with length (a0/d0) into its 1024-byte buffer without a
; limit; the pens apen/bpen follow the buffer.  A string of 1026
; characters puts "xx" into apen; the next text is drawn with that pen
; (FrontPen = its low byte, 120) instead of 1.  (The original takes the
; pens before the copy: the long string itself is still drawn in pen 1.)
; Made safe: the overflow is 3 bytes (the pens only); the bitmap is
; 320 x 256 x 8, so the unclipped long text (8208 pixels wide, it runs on
; through the rows of the plane) stays inside its planes.  The bitmap is
; cleared, a short text printed, and the program prints the highest pen
; it finds in the first 16 x 16 pixels.
; Correct: the string is cut to the buffer, the pens stay: pen 1.
; @rwe riintuilib
BitMap 0,320,256,8
IPBitMap 0,0
IPOutput 0
Print String$("x",1026)
Cls 0
Print "x"
DefaultOutput
p=0
For y=0 To 15
  For x=0 To 15
    If Point(x,y)>p Then p=Point(x,y)
  Next
Next
NPrint "pen ",p
End
