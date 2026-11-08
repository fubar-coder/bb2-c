; compiler-object-index-long (#939) - every form that addresses an object
; as table base + n << shift, with a maximum whose last object lies past
; $7FFF: the BitMap maximum is raised to 513 (bitmaplib = 160, objects of
; 64 bytes), BitMap 512 is 32768 bytes behind BitMap 0.  The forms: a
; library command's object parameter (BitMap n,..., libcall toreg), Use
; (douse), Free (dofree) and Addr (doaddr, also as the right operand of
; an expression, in another data register).  o is the object's address
; computed as a long.
; Correct: Addr BitMap(512) - Addr BitMap(0) = 32768; BitMap 512 sets the
; bytes per row of that object (2); Use BitMap 512 makes Plot draw into
; its plane ($80 in the first byte); Free BitMap 512 clears its "own
; memory" field (+62).
; @fixed #939
; @pass jobline=max=160|513
DEFTYPE .l
o=Addr BitMap(0)+32768
NPrint Addr BitMap(512)-Addr BitMap(0)
NPrint 4+Addr BitMap(512)-o
BitMap 512,16,8,1
NPrint Peek.w(o)," ",Peek.w(o+62)
BitMap 0,16,8,1
Use BitMap 512
Plot 0,0,1
NPrint Peek.b(Peek.l(o+8)) AND 255
Free BitMap 512
NPrint Peek.w(o+62)
End
