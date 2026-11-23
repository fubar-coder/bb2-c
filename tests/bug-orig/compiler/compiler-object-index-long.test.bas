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
DEFTYPE .l

!TestCase{object_index_long}
  o=Addr BitMap(0)+32768
  r=Addr BitMap(512)-Addr BitMap(0)
  !CheckEq{r,32768,"addr_difference"}
  r=4+Addr BitMap(512)-o
  !CheckEq{r,4,"addr_second_register"}
  BitMap 512,16,8,1
  r=Peek.w(o)
  !CheckEq{r,2,"bytes_per_row"}
  r=Peek.w(o+62)
  !CheckEq{r,-1,"own_memory"}
  BitMap 0,16,8,1
  Use BitMap 512
  Plot 0,0,1
  r=Peek.b(Peek.l(o+8)) AND 255
  !CheckEq{r,128,"plotted_byte"}
  Free BitMap 512
  r=Peek.w(o+62)
  !CheckEq{r,0,"own_memory_freed"}
!EndTestCase
