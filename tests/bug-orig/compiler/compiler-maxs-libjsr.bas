; compiler-maxs-libjsr (#335) - blitz2 builds the table of the object
; arrays memlib allocates at the start (calcmaxs) before it resolves the
; calls inside the linked library code (fixjsrs): an object library only
; reached from another library gets no array, its base stays 0, but End
; still frees all its objects.  blitzoslib calls Blit and so links
; shapeslib; this program has no shapes command (InitPage behind End is
; never run, linking blitzoslib is enough).  At End 100 "shapes" from
; address 0 are freed: the system hangs.
; Correct: the program ends normally.
; @pass timeout=40
NPrint "x"
End
InitPage 0,0,2
