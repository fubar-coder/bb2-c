; popupwinlib-v36-calls (#1140) - the library's init creates the message
; port of the menu windows with CreateMsgPort (exec V36) without checking
; exec's version; on Kickstart 1.3 (exec V34) the offset lies behind
; exec's jump table: every program with the library crashes at start-up,
; before its first line (the session ends in a timeout).  The menu levels
; also call AllocVec/FreeVec (exec V36) and OpenWindowTagList (intuition
; V36), finit DeleteMsgPort.
; Here: the program starts, PopUpInit, and it ends (finit frees the port).
; The menu itself: popupwinlib-v36-menu.
; Correct: the program runs on every machine (the library builds its port
; with exec V33 functions) and ends normally.
; @fixed #1140
; @rwe popupwinlib
; @machine a500 a500-204 a1200
; @pass timeout=30
NPrint "before"
PopUpInit 0
NPrint "after"
End
