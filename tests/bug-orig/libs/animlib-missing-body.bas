; animlib-missing-body (#400) - LoadAnim stores the result of
; ifffind("BODY") as the BODY length without testing "not found": iffiolib
; returns -1, the library allocates and reads -1 bytes (memlib's header
; makes it AllocMem(7)), and InitAnim would decode whatever follows that
; block.
; Here: an ANIM (generated) whose first frame has a BMHD but no BODY;
; only LoadAnim, the program reads the BODY length of the object (+14).
; Correct: a missing BODY is not taken as a BODY of $FFFFFFFF bytes.
; @configs clib
; @pass file=nobody.anim|@TESTDATA@/animlib-missing-body.anim
DEFTYPE .l
LoadAnim 0,"CORPUS:nobody.anim"
a=Addr Anim(0)
NPrint "width ",Peek.w(a)," frames ",Peek.w(a+8)
NPrint "body length -1 ",Peek.l(a+14)=-1
End
