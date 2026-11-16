; brexxlib-playwait (#99) - PlayWait Tape# plays only while a playback
; runs: it restarts the playback with the given tape (with the end
; signal) and waits for the signal.  With an empty tape it starts
; nothing, the running playback has no end signal, and PlayWait waits
; for ever.
; Here: a tape of two events (TapeTrap, QuietTrap On), PlayBack of it,
; then PlayWait with an empty tape.
; Correct: PlayWait returns (nothing to restart).  The original hangs.
; @fixed #99
; @configs clib
; @pass timeout=30
QuietTrap On
TapeTrap 0
AbsMouse 40,30
AbsMouse 60,40
QuietTrap Off
PlayBack 0
PlayWait 1
NPrint "returned"
End
