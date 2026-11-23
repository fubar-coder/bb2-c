; dhandlerlib-no-cache-flush (#409) - RUN from the editor with the
; debugger: the init of dhandlerlib copies the code template of the
; debugger process into a new memory block, patches two longs in the
; copy and starts it with CreateProc ("Blitz ][ Debug Proc") without a
; cache flush in between: on a 68020+ with caches (68040/060 copyback)
; the process may run stale code.  Amiberry does not show the effect, so
; inject watches the cause: did the program's process call
; CacheClearU/CacheClearE before that CreateProc (cacheflush=)?  The
; driver runs from the editor (editor-run, tedstub's COMPILE/RUN) with
; runtime error checking on and the CD's debugger program as
; blitz2:defaultdbug, so the program's init starts the debugger process.
; Correct: the caches are flushed before the process is started (the
; query compile.inject: inject runs the compiler, whose process runs the
; program).
!TestCase{debug_proc}
  !CheckEq{6*7,42,"ran"}
!EndTestCase
