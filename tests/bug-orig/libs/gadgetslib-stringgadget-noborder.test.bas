; gadgetslib-stringgadget-noborder (#164) - after Borders Off,
; allocborder gives the border pointer 0, and StringGadget still writes
; the border offsets (-x and -y excess) to the LeftEdge and TopEdge of its
; two borders: the words at the addresses 0, 2, 36 and 38.
; Here: with interrupts off (Disable) the program saves the four words,
; puts known values there, makes the string gadget, reads them and puts
; the old words back.
; Correct: without borders nothing is written there: the words still hold
; the known values.  The original writes 0 to all four (the negated border
; excess, which Borders Off sets to 0).
DEFTYPE .l

!TestCase{no_borders}
  Borders Off
  Disable_
  s0=Peek.l(0) : s1=Peek.l(36)
  Poke.w 0,$1111 : Poke.w 2,$2222 : Poke.w 36,$3333 : Poke.w 38,$4444
  StringGadget 0,10,20,0,1,20,100
  a=Peek.w(0) : b=Peek.w(2) : c=Peek.w(36) : d=Peek.w(38)
  Poke.l 0,s0 : Poke.l 36,s1
  Enable_
  !CheckEq{a,$1111,"word_0"}
  !CheckEq{b,$2222,"word_2"}
  !CheckEq{c,$3333,"word_36"}
  !CheckEq{d,$4444,"word_38"}
  !CheckEq{Peek.l(Peek.l(Addr GadgetList(0))+18),0,"render"}
!EndTestCase
