; gtlib-getoptions (#174) - getoptions (the labels of GTCycle/GTMX) gets
; a block of option length + 258 bytes (memlib globalloc): the pointer
; array at +0, the texts at +256, room for 63 labels + NULL, not checked;
; the block is never freed, not even by Free GTList.
; The program runs through inject with guard (runwrap=): a pattern behind
; every AllocMem, checked at FreeMem and at the end.  On the Workbench
; screen a GTCycle with 100 empty labels (99 "|": 400 bytes of pointers
; in a block of 357), the number of labels GadTools has (GTCY_Labels via
; GTGetAttrs, up to the NULL); then 20 times a GTCycle with 1000 bytes of
; labels and Free GTList, the free memory (exec AvailMem) before and
; after.
; Correct: 100 labels, nothing written behind an allocation, the free
; memory does not shrink by more than 8000 bytes.  The original writes
; the pointers over the texts (zeros end the array early) and behind its
; block, and keeps about 1300 bytes per GTCycle.
; @fixed #174
; @configs clib
; @pass runwrap=@INJECT@|guard
DEFTYPE .l
WbToScreen 0
o$=String$("|",99)
GTCycle 0,1,10,10,100,14,"A",0,o$
p=GTGetAttrs(0,1,$8008000E)
n=0
If p
  While Peek.l(p+n*4)<>0 AND n<200 : n=n+1 : Wend
EndIf
NPrint "labels ",n
Free GTList 0
o$=String$("a|",500)
MOVE.l 4,a6 : MOVEQ #0,d1 : JSR -216(a6) : PutReg d0,m1
For i=1 To 20
  GTCycle 0,1,10,10,100,14,"A",0,o$
  Free GTList 0
Next
MOVE.l 4,a6 : MOVEQ #0,d1 : JSR -216(a6) : PutReg d0,m2
NPrint "lost under 8000 ",m1-m2<8000
End
