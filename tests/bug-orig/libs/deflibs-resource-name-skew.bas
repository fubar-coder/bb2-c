; deflibs-resource-name-skew (#45) - the inits of the eight resource
; libraries of the deflibs (ciaa, ciab, disk, misc, potgo, battclock,
; battmem, card) open a name shifted by 6 bytes ("ciaa.resource" ->
; "esource"), which never exists, and retry forever (#44): a program that
; uses one of their calls hangs at startup.
; Here, one harmless call of each resource (the A1200 has all eight),
; between two lines of output.
; Correct: the program starts, the calls work (the bases are right), it
; ends.  The original hangs before the first line (the session ends by its
; time limit).
; @configs clib
; @pass timeout=30
DEFTYPE .l
NPrint "start"
x=AbleICR_a(0,0)
x=AbleICR_b(0,0)
x=GetUnitID_(0)
x=AllocMiscResource_(3,"bug-orig")
If x=0 Then FreeMiscResource_ 3
x=AllocPotBits_(0)
x=ReadBattClock_
ObtainBattSemaphore_
ReleaseBattSemaphore_
x=CardInterface_
NPrint "end"
End
