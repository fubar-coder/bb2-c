; rimisclib-ezrequest-args-unbounded (#748) - EZRequest title$,body$,
; gadget$,var.. copies every argument into ezargs2 (16 longs) without a
; limit (a long argument takes a long, a word or byte one a word).  In
; the original the code of WorkbenchHere follows the array: 18 long
; arguments ($4AFC4AFC, ILLEGAL) overwrite its first 8 bytes,
; the call of WorkbenchHere afterwards crashes (the session ends in a
; timeout).  The program opens a screen "Workbench" for WorkbenchHere to
; find (the emulated system has none open).  WorkbenchHere takes
; IntuitionBase from d0 (#759): the program loads it with GetReg.
; Correct: the arguments beyond 16 are not stored, WorkbenchHere returns.
; @rwe rimisclib
; @pass timeout=30
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
