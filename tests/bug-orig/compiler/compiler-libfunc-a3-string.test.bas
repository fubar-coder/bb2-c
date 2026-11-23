; compiler-libfunc-a3-string (#46) - a library function whose !libs list
; loads its object into a3 ($1380: BlitColl's Shape, PeekSound's Sound)
; inside a string expression: a3 is also the end of the string being
; built, and blitz2 does not save it around the call, so the rest of the
; expression writes its characters from the object on.  Here " b" and
; its terminator land on the shape's width and height words and " d" on
; the sound's data pointer (both put back before the end, which frees
; them).  The NPrint lines are the acts (their output is not judged).
; Correct: the objects are unchanged (width 20, height 9; the sound's
; first long the same).
!TestCase{a3_string}
  BitMap 0,64,32,2 : Use BitMap 0
  GetaShape 1,8,4,20,9
  BitMap 1,96,48,3 : Use BitMap 1
  a.l=Addr Shape(1) : w.l=Peek.l(a)
  w0.l=Peek.w(a) : h0.l=Peek.w(a+2)
  !CheckEq{w0,20,"width_before"}
  !CheckEq{h0,9,"height_before"}
  NPrint "a ",BlitColl(1,40,20)," b"
  w1.l=Peek.w(a) : h1.l=Peek.w(a+2) : sw.l=ShapeWidth(1)
  !CheckEq{w1,20,"width_after"}
  !CheckEq{h1,9,"height_after"}
  !CheckEq{sw,20,"shapewidth_after"}
  Poke.l a,w
  InitSound 0,32
  s.l=Addr Sound(0) : p.l=Peek.l(s)
  NPrint "c ",PeekSound(0,1)," d"
  u.l=(Peek.l(s)=p)
  !CheckEq{u,-1,"sound_unchanged"}
  Poke.l s,p
!EndTestCase
