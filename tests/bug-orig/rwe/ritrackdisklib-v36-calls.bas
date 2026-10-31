; ritrackdisklib-v36-calls (#941) - the library's init calls CreateMsgPort
; (exec V36) without checking exec's version; on Kickstart 1.3 (exec V34)
; the offset lies behind exec's jump table: every program with the
; library crashes at start-up, before its first line (the session ends in
; a timeout).
; Correct: the program runs on Kickstart 1.3 (the commands fail, OpenDisk
; returns 0) and ends normally.
; @rwe ritrackdisklib
; @machine a500
; @pass timeout=30
NPrint "before"
r.l=OpenDisk(0)
NPrint "after"
End
