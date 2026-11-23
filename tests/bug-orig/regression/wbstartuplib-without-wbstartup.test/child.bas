; wbstartuplib-without-wbstartup.test/child.bas: the program of the test,
; started the Workbench way by the case; it has no WBStartup, its first DOS
; call is Lock_
DEFTYPE .l
l=Lock_("SYS:",-2)
If l Then UnLock_ l
End
