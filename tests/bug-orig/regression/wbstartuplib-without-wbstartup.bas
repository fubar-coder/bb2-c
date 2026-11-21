; wbstartuplib-without-wbstartup - a program started from the Workbench
; without the command WBStartup (wbrun starts it like the Workbench does).
; The manual (BLITZ2_MANUAL, R-9) says such a program "will promptly
; crash".  No other library takes the WBStartup message from the process's
; port; it stays there, unreplied, in front of every later message.  At
; its first DOS call that needs the file system (Lock_ here) the process
; waits for the reply to its packet, takes the WBStartup message for that
; reply, and DOS stops the program with the requester "Software Failure"
; (error 87000004, a dead end, gadgets "Suspend" and "Reboot").  The
; Kickstart 3.1 ROM does that for any program that leaves the message
; there (also a C program, also the original compiler's executable).
; reqtrap (the launcher) records the requester and answers "Suspend", the
; process stays suspended; wbrun waits 5 s for the reply, says that none
; came and that the process still runs, and waits; the run ends by the hang
; check of the runner (session timeout).  The session of this test is its
; own (one emulator session per test): the suspended process is not seen
; by another test.
; The expectation records what the port does today; the program is wrong
; (the manual says so), this is no bug.
; @pass runwrap=@WBRUNT@
DEFTYPE .l
l=Lock_("SYS:",-2)
If l Then UnLock_ l
End
