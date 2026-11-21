; wbstartuplib-without-wbstartup - a program started from the Workbench
; without the command WBStartup (wbrun starts it like the Workbench does).
; The manual (BLITZ2_MANUAL, R-9) says such a program "will promptly
; crash".  No other library takes the WBStartup message from the process's
; port; it stays there unreplied.  A DOS function that waits for a packet
; (Lock_ here) never returns (DOS waits on that port, where the message
; lies: the likely reason, not checked).  wbrun waits 5 s for the reply,
; says that none came and that the process still runs, and waits; the run
; ends by the hang check of the runner (session timeout).
; The expectation records what the port does today; the program is wrong
; (the manual says so), this is no bug.  Started by the default launcher
; (reqtrap) instead, the run ends in a storm of requesters or a halt of the
; CPU, which is why wbrunt is the launcher here.
; @pass launcher=@WBRUNT@
DEFTYPE .l
l=Lock_("SYS:",-2)
If l Then UnLock_ l
End
