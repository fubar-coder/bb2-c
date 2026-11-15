; bblitlib-plane-step-signed (#1045) - BBlit saves the background into a
; record of the buffer, plane after plane: the plane address and the
; bytes saved per plane (shape +24: (bytes per row + 2) x height).  The
; step to the next plane is that word added sign-extended (ADD.w), so for
; 32K or more bytes per plane it goes backwards; UnBuffer and ShowStencil
; step the same way.  Here a 320x800 shape (33600 bytes per plane) on a
; bitmap of one plane: the record fits the buffer, only the write
; position after it is wrong (a further plane or BBlit would write before
; the buffer, so the test stops there).
; Correct: the write position is 16 + 4 + 33600 = 33620 bytes behind the
; start of the buffer.
; @fixed #1045
DEFTYPE .l
BitMap 0,352,800,1
Use BitMap 0
GetaShape 0,0,0,320,800
Buffer 0,70000
b0=Addr Buffer(0)
NPrint "bytes per plane ",Peek.w(Addr Shape(0)+24) AND $FFFF
BBlit 0,0,0,0
NPrint "write position ",Peek.l(b0)-Peek.l(b0+8)
UnBuffer 0
NPrint "after UnBuffer ",Peek.l(b0)-Peek.l(b0+8)," ",Peek.l(b0+4)
End
