; dhandlerlib-no-cache-flush (#409) - RUN from the editor with the
; debugger: the init of dhandlerlib copies the code template of the
; debugger process into a new memory block, patches two longs in the
; copy and starts it with CreateProc ("Blitz ][ Debug Proc") without a
; cache flush in between: on a 68020+ with caches (68040/060 copyback)
; the process may run stale code.  Amiberry does not show the effect, so
; inject watches the cause: did the program's process call
; CacheClearU/CacheClearE before that CreateProc (cacheflush=)?  tedstub
; runs the program with COMPILE/RUN, runtime error checking on (the
; debugger, dhandlerlib).
; Correct: the caches are flushed before the process is started.
; @configs clib
; @pass runtime_errors=on
; @pass wrap=@INJECT@|cacheflush=Debug
; @pass jobline=run=r001|CORPUS:src/dhandlerlib-no-cache-flush.bb2|RAM:con
NPrint "hello"
End
