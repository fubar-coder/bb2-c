; polylib-bitmappoly-points-check (#395) - BitMapPoly (also ChunkyPoly
; and RenderPoly) copies the points twice into a buffer of 32 points
; without checking their number: more than 16 points overwrite what
; follows it (in the original the code of the library).  Such a count is
; a programming error, and the original has no error check for it, not
; even with runtime error checking.
; Here with runtime error checking: BitMapPoly with 16 points (no error;
; they run anticlockwise: the reverse form, a negative count), then with
; 17 (the same polygon with one more point on its left edge).
; Correct: the error check refuses the 17 points: "Too Many Points".
; The error is announced with !ExpectError; the original stops with a
; crash requester instead (see the configuration).
DEFTYPE .l
Statement setp{m.l,a$}
  While a$<>""
    i.l=Instr(a$,",")
    If i=0 Then i=Len(a$)+1
    Poke.w m,Val(Left$(a$,i-1)) : m=m+2
    a$=Mid$(a$,i+1)
  Wend
End Statement

!TestCase{points_17}
  BitMap 0,320,200,1
  Dim p.w(100)
  a=&p(0)
  setp{a,"55,2,10,10,10,20,10,30,10,40,10,50,10,60,10,70,10,80,10,90,10,100,10,110,10,120,10,130,100,140,100,10"}
  BitMapPoly -16,a,1
  !Check{1,"sixteen_points"}
  setp{a,"55,2,10,10,10,20,10,30,10,40,10,50,10,60,10,70,10,80,10,90,10,100,10,110,10,120,10,125,10,130,100,140,100,10"}
  !ExpectError{"Too Many Points","refused"}
  BitMapPoly -17,a,1
  !Check{0,"after"}
!EndTestCase
