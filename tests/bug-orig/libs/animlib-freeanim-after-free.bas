; animlib-freeanim-after-free (#399) - freeanim (Free Anim, LoadAnim over
; a loaded anim, the program end) walks the delta nodes and reads the
; next pointer from a node after it has freed it (memlib globfree, exec
; FreeMem).  Normally the old contents are still there; a memory checker
; or another task reusing the block makes the walk follow garbage.
; Here: the program runs under inject freefill=FF, which sets every block
; of the process to $FF in its FreeMem, as a memory checker does; an ANIM
; with one delta (generated), LoadAnim, Free Anim.
; Correct: the next pointer is read before the node is freed: Free Anim
; frees the node and the walk ends, the program goes on.  The original
; follows the pointer $FFFFFFFF.
; @fixed #399
; @configs clib
; @pass file=onedelta.anim|@TESTDATA@/animlib-freeanim-after-free.anim
; @pass runwrap=@INJECT@|freefill=FF
DEFTYPE .l
LoadAnim 0,"CORPUS:onedelta.anim"
NPrint "frames ",Frames(0)
Free Anim 0
NPrint "freed"
End
