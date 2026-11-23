; compiler-macrobuff-nomem (#1176, a bug of the port) - makemacbuff (the
; fix of #336) allocates the Macro Buffer anew when the options change it
; (loadxtra, optsdone).  When that allocation fails, the old buffer stays
; and nothing tells the user that the new size did not take effect.  The
; defaults of tedstub have a Macro Buffer of 20000 bytes; the allocation
; of 20000 bytes (MEMF_PUBLIC) fails once, in the loadxtra of the job.
; Correct (D-018): the out of memory notice - from tedstub, which has no
; TED-C notice, blitz2's own error "Out of Memory" outside the compile
; (XTRAERR) - and the old buffer of 8192 bytes stays: the program with
; a macro of 70 bytes compiles and runs.
Macro m
a$="xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
End Macro

!TestCase{old_buffer}
  !m
  !CheckEq{Len(a$),70,"macro_text"}
!EndTestCase
