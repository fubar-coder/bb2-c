; rimapshapeslib-addgroupmember-before-resetgroup (#901) - AddGroupMember
; writes the member {map shape address, dx, dy} at the next-member pointer
; and the end mark behind it; only ResetGroup sets that pointer, init
; leaves it 0.  An AddGroupMember before the first ResetGroup writes the
; member to addresses 0-7 (dx/dy over the ExecBase pointer at 4) and the
; end mark to 8-11.
; Made safe: interrupts off (Disable_) from before AddGroupMember until the
; three longs at 0, 4 and 8 are put back.
; Correct: the member goes into the (empty) group: addresses 0-11 stay.
; @fixed #901
; @rwe rimapshapeslib
DEFTYPE .l
SetNumMapShapes 10
Disable_
o0=Peek.l(0) : o4=Peek.l(4) : o8=Peek.l(8)
AddGroupMember 3,1,1
n0=Peek.l(0) : n4=Peek.l(4) : n8=Peek.l(8)
Poke.l 0,o0 : Poke.l 4,o4 : Poke.l 8,o8
Enable_
If n0=o0 AND n4=o4 AND n8=o8 Then NPrint "addresses 0-11 kept" Else NPrint "addresses 0-11 written"
End
