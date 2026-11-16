; brexxlib-xstatus-bit15 (#103) - XStatus returns the mode word as it
; is, with bit 15, the input handler's "inject the event first / take
; the start time" flag: $8001 (32769) right after Record Tape#, $8000
; (32768) right after an executed simple event, until the input handler
; runs the next time.
; Correct: 1 while recording, 0 with nothing recorded or played.  The
; original prints 32769 and 32768.
; @fixed #103
; @configs clib
Record 0
NPrint "record ",XStatus
Record
AbsMouse 40,30
NPrint "event ",XStatus
End
