; wbstartuplib-without-wbstartup-end - a program started from the
; Workbench without the command WBStartup that calls no DOS function: it
; just ends.  The WBStartup message stays unreplied in the process's port
; (wbstartuplib-without-wbstartup: the first DOS call that waits for a
; packet stops the program).  The program's code ends normally, but the
; process does not go away quietly: DOS frees a lock of the process at its
; exit (packet type 15, ACTION_FREE_LOCK; wbrun gives the process a
; current directory and a home directory lock), and its wait for the reply
; takes the WBStartup message for it: the requester "Software Failure"
; (error 87000004; gadgets "Suspend" and "Reboot").  The launcher reqtrap
; (with its option -harden, set by launcherargs in the toml: without it
; the requester gets the answer "Reboot", which halts the machine) records
; it and answers "Suspend"; wbrunt, whose exit hook has run by then, says
; "no reply, the process ended".  The manual's "will promptly crash"
; (BLITZ2_MANUAL, R-9) shows for such a program too, only after its end.
; Not checked: whether a process of the real Workbench has a lock that DOS
; frees at its exit.
; The program is not the driver: the driver's DOS calls (the result file)
; would stop it at once.  The case starts the child
; (wbstartuplib-without-wbstartup-end.test/child.bas) the Workbench way
; with wbrunt (the files of the toml stage it as CORPUS:wbrunt).  The
; expectation records what the port does today.
DEFTYPE .l

!TestCase{child}
  *r.TLRun=!RunProgram{"CORPUS:wbrunt @TESTDIR@/child","child"}
  !CheckStrEq{*r\printed,"wbrun: no reply, the process ended"+Chr$(10),"printed"}
!EndTestCase
