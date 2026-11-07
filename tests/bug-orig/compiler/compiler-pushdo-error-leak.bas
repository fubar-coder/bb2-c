; compiler-pushdo-error-leak (#994) - pushdo saves the code of an
; assignment's address (here the index of a(i)) into a buffer from
; doallocmem, and pushput puts it back after the expression and frees the
; buffer. A compile error in the expression leaves the compiler without
; reaching pushput: the buffer is lost. inject records blitz2's
; allocations (track).
; Correct: the compile stops at the line of the error and blitz2 leaves
; no allocation behind.
; @fixed #994
; @pass wrap=@INJECT@|track
Dim a(10)
i=1
a(i)=1+"x"
End
