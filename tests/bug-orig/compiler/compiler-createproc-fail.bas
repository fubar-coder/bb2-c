; compiler-createproc-fail (#25) - COMPILE/RUN with the editor open: menu10
; opens the stop requester "EXECUTING" and starts the program with
; CreateProc; when CreateProc fails it jumps past endstop, the requester
; stays open (and there is no error message).  inject makes CreateProc of
; blitz2 fail, tedstub runs the program with COMPILE/RUN and logs the
; requesters still open behind it.
; Correct: no requester is left open.
; @pass wrap=@INJECT@|createprocfail
; @pass jobline=run=r001|CORPUS:src/compiler-createproc-fail.bb2|RAM:con
NPrint "hello"
End
