; compiler-wbexit-returncode (#22) - blitz2 ends with returncode in d0 and
; jumps to wb_exit, which loads wbmess into d0 first: started from the
; shell, blitz2 always returns 0.  Here blitz2:ted is missing, the
; compiler gives up at once with returncode -1 (blitz2.s:1369) - and
; returns 0.  inject (no faults) runs blitz2 and shows the value it
; returns as is (the shell would show -1 as 10).
; Correct: blitz2 returns -1 (and compiles nothing).
; @fixed #22
; @pass remove=blitz2/ted
; @pass wrap=@INJECT@
End
