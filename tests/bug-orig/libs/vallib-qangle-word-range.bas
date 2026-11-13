; vallib-qangle-word-range (#1003) - QAngle subtracts the coordinates as
; words and takes the sign and the absolute value (NEG) of the word: a
; difference of 32768 or more in magnitude wraps (the word $8000 is
; -32768 and +32768 at once, NEG leaves it negative), so the angle is that
; of another quadrant or half plane.
; Here QAngle for differences of 32767, 32768 and 65535 along the axes
; and the diagonal.
; Correct: the angle of the same direction with small differences, e.g.
; QAngle(0,0,-32768,0) = QAngle(0,0,-10,0) = $8001.
; @fixed #1003
Statement pa{x.w,y.w,u.w,v.w}
  a.q=QAngle(x,y,u,v)
  NPrint "qangle ",x," ",y," ",u," ",v," ",Hex$(Peek.l(&a))
End Statement
pa{0,0,32767,0} : pa{0,0,-32768,0} : pa{-32768,0,0,0}
pa{0,0,0,-32768} : pa{0,-32768,0,0} : pa{0,0,-32768,-32768}
pa{32767,0,-32768,0} : pa{-32768,0,32767,0} : pa{0,32767,0,-32768}
pa{32767,32767,-32768,-32768}
End
