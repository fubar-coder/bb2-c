; risoundlib2-v36-calls (#1139) - the library's init calls CreateMsgPort
; and CreateIORequest (exec V36) without checking exec's version; on
; Kickstart 1.3 (exec V34) the offsets lie behind exec's jump table: every
; program with the library crashes at start-up, before its first line
; (the session ends in a timeout).
; Correct: the program runs on Kickstart 1.3 (the commands fail, OpenAHI
; returns 0) and ends normally.
; @fixed #1139
; @rwe risoundlib2
; @machine a500
; @pass timeout=30
NPrint "before"
r.l=OpenAHI(0)
NPrint "open ",r
NPrint "after"
End
