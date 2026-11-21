; wbstartuplib-without-wbstartup-end - a program started from the
; Workbench without the command WBStartup that calls no DOS function: it
; just ends.  The WBStartup message stays unreplied in the process's port
; (wbstartuplib-without-wbstartup: a DOS call then hangs), but nothing
; asks for it: the process ends normally, wbrun reports that no reply came
; and that the process ended.  The manual's "will promptly crash"
; (BLITZ2_MANUAL, R-9) does not show for such a program.
; The expectation records what the port does today.
; @pass launcher=@WBRUNT@
End
