; shapeslib-initshape-depth-zero (#243, #74) - InitShape has no error
; check (BLITZ_SUBS(_initshape,0,0)): with depth 0 it makes a shape of
; depth 0 with a cookie (AutoCookie is on), which the blit commands'
; checks pass (maxchk tests only the cookie), and their plane loops (DBF
; on depth-1) then blit 65536 planes.
; Here with runtime error checking: InitShape 0,16,8,0, then the depth of
; the shape (+4).  Nothing is blitted.
; Correct: an error check refuses the depth: "Illegal Shape Depth", the
; command is not done.
; @fixed #243
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
NPrint "before"
InitShape 0,16,8,0
NPrint "after, depth ",Peek.w(Addr Shape(0)+4)
End
