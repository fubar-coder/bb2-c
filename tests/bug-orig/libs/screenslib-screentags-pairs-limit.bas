; screenslib-screentags-pairs-limit - ScreenTags Screen#,Title$,Tag,Data
; [,Tag,Data...] copies its tag pairs from the argument stack into the
; static usertags (32 longs = 16 pairs) without a limit, then a TAG_END
; long: 16 pairs or more write behind the buffer (in the original the
; error check area, without the debugger the code of the next library).
; The &TagList form is fixed (#249); these pairs lie on the argument
; stack in reverse order and cannot be linked with TAG_MORE, as with
; windowslib's WindowTags (#961).
; An invalid argument: the error check should catch it (BB2 style).
; Runtime error checking on (the debugger); ScreenTags with 16 pairs
; (TAG_IGNORE).
; Correct: the error check stops it with "Too Many Tags".  The original
; has no check and opens the screen.
; @pass runtime_errors=on
DEFTYPE .l
t=1
s=ScreenTags(0,"Tags",t,0,t,0,t,0,t,0,t,0,t,0,t,0,t,0,t,0,t,0,t,0,t,0,t,0,t,0,t,0,t,0)
NPrint "opened ",s<>0
CloseScreen 0
End
