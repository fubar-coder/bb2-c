; rimisclib-ezrequest-args-unbounded (#748) - EZRequest title$,body$,
; gadget$,var.. copies every argument into ezargs2 (16 longs) without a
; limit (a long argument takes a long, a word or byte one a word).  In
; the original the code of WorkbenchHere follows the array: 18 long
; arguments ($4AFC4AFC, ILLEGAL) overwrite its first 8 bytes,
; the call of WorkbenchHere afterwards crashes.  The program opens a
; screen "Workbench" for WorkbenchHere to find (the emulated system has
; none open).  WorkbenchHere takes IntuitionBase from d0 in the original
; (#759): the program loads it with GetReg.
; An invalid argument (the number of arguments is in the source): the
; error check catches it (BB2 style), the command itself is unchanged.
; Runtime error checking on (the debugger).
; Correct: the error check stops EZRequest with "Too Many EZRequest
; Arguments".  The original has no check.
; @fixed #748
; @rwe rimisclib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
q=$4AFC4AFC
Screen 0,0,0,320,200,2,0,"Workbench",1,2
EZRequest "BO","x","OK",q,q,q,q,q,q,q,q,q,q,q,q,q,q,q,q,q,q
ib=OpenLibrary_("intuition.library",0)
NPrint "before"
GetReg d0,ib
r=WorkbenchHere
CloseLibrary_ ib
NPrint r
End
