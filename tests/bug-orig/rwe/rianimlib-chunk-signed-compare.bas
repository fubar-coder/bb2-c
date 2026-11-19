; rianimlib-chunk-signed-compare (#506) - the chunk walks compare the
; addresses signed: a chunk length that moves the pointer past
; $7FFFFFFF makes it "below" the end, and the walk goes on reading chunk
; headers from there instead of stopping.  The ANIM lies in Z3 fast
; memory ($40000000 on) and has two chunks: the first frame and a JUNK
; chunk that claims $40000000 bytes.  RIAnimFrameCount's walk steps past
; the JUNK chunk to $80000000 and above (no memory there) and keeps
; counting the "chunks" it finds.
; Correct: the walk stops at the end of the ANIM, the count is 1 (the
; frame; the JUNK chunk is no frame, #510, and passes the end of the
; ANIM; the wrong count depends on what the emulator returns for the
; unmapped addresses, so it is only reported as wrong).
; @fixed #506
; RIPackLib calls RIAmosFuncLib.
; @rwe rianimlib ripacklib riamosfunclib
; The walk is silent for about 6 s alone and up to 10 s under load (more
; than BUG_ORIG_RUN_HANG_AFTER):
; @pass hang_after=30
; @pass file=anim|@TESTDATA@/rianimlib-chunk-signed-compare.anim
DEFTYPE .l
a=AllocMem_(4096,$10005)
f=Open_("CORPUS:anim",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,a,4096)
Close_ f
If a<$40000000 Then NPrint "not in Z3 memory" : End
InitPalette 0,2                 ; palettelib linked by the program itself (#335)
BitMap 0,16,16,1                ; the bitmap the first frame is unpacked into
r=RIAnimInit(a,0,0)
c=RIAnimFrameCount
If c=1
  NPrint "count 1"
Else
  NPrint "count wrong"
EndIf
FreeMem_ a,4096
End
