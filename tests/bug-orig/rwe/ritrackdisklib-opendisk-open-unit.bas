; ritrackdisklib-opendisk-open-unit (#862) - OpenDisk of a unit that is
; open makes a new request and opens the device again: it returns -1, and
; the first request stays open and is lost (CloseDisk closes only the new
; one).
; Correct: an open unit is left alone, the second OpenDisk returns 0; one
; CloseDisk closes the unit.
; @rwe ritrackdisklib
NPrint OpenDisk(0)
NPrint OpenDisk(0)
NPrint CloseDisk(0)
NPrint CloseDisk(0)
End
