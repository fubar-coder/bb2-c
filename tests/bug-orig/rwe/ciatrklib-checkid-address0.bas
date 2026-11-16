; ciatrklib-checkid-address0 (#430) - CheckTrackerID returns -1 for a
; module with the ID "M.K." or "M!K!" and otherwise MOVE.l 0,d0: the long
; at ADDRESS 0 instead of 0 (False).  The program puts $12345678 at
; address 0 during the calls (and restores it): a slot whose memory has no
; ID (2200 zero bytes given with DecodeModule) and an empty slot.
; Correct: both are False (0); a module with "M.K." is True (-1).
; @fixed #430
; @rwe ciatrklib
DEFTYPE .l
Dim m.b(2200)
a=&m(0)
DecodeModule 0,a
z=Peek.l(0) : Poke.l 0,$12345678
r0=CheckTrackerID(0)
r3=CheckTrackerID(3)
Poke.l 0,z
NPrint "no id ",r0
NPrint "empty ",r3
Poke.l GetTrackerLocation(0)+$438,$4D2E4B2E
NPrint "m.k. ",CheckTrackerID(0)
FreeTrackerModule 0
End
