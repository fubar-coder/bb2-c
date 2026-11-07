; compiler-flushlibs-leak (#26) - at its end blitz2 asks for 256 MB
; (flushlibs) so that exec removes unused libraries; the request is meant
; to fail, its result is neither checked nor freed.  The emulator gets
; 512 MB Z3 RAM (uae=), so the request succeeds; inject track reports the
; allocation that is left.
; Correct: nothing of blitz2 is left allocated.
; @fixed #26
; @pass uae=z3mem_size=512
; @pass wrap=@INJECT@|track
NPrint "hello"
End
