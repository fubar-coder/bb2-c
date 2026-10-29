; compiler-wb-exit-permit (#337) - started from the Workbench, blitz2
; answers the WBStartup message in wb_exit with Forbid, ReplyMsg, Permit
; and only then returns: after the Permit the Workbench (priority 1, the
; tool 0) runs at once and may unload blitz2 while its last instructions
; are still to be executed.  wbrun starts blitz2 like the Workbench, with
; the job list as project icon, and with freeseg overwrites and frees the
; seglist right after the reply, as a memory checker (MungWall) or a
; reuse of the memory would; blitz2 then runs into the overwritten code.
; The port does not have the code (libnix answers without Permit).
; Correct: blitz2 has ended when the reply arrives, the session ends
; normally.
; @pass wrap=@WBRUN@|freeseg
NPrint "hello"
End
