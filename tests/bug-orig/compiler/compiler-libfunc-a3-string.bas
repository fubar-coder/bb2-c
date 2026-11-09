; compiler-libfunc-a3-string (#46) - a library function whose !libs list
; loads its object into a3 ($1380: BlitColl's Shape, PeekSound's Sound)
; inside a string expression: a3 is also the end of the string being
; built, and blitz2 does not save it around the call, so the rest of the
; expression writes its characters from the object on.  Here " b" and
; its terminator land on the shape's width and height words and " d" on
; the sound's data pointer (both put back before the end, which frees
; them).  Correct: the objects are unchanged (width 20, height 9; the
; sound's first long the same), the lines are printed as they read.
; @fixed #46
BitMap 0,64,32,2 : Use BitMap 0
GetaShape 1,8,4,20,9
BitMap 1,96,48,3 : Use BitMap 1
a.l=Addr Shape(1) : w.l=Peek.l(a)
NPrint "before ",Peek.w(a)," ",Peek.w(a+2)
NPrint "a ",BlitColl(1,40,20)," b"
NPrint "after ",Peek.w(a)," ",Peek.w(a+2)," ",ShapeWidth(1)
Poke.l a,w
InitSound 0,32
s.l=Addr Sound(0) : p.l=Peek.l(s)
NPrint "c ",PeekSound(0,1)," d"
If Peek.l(s)=p Then NPrint "sound unchanged" Else NPrint "sound changed"
Poke.l s,p
End
