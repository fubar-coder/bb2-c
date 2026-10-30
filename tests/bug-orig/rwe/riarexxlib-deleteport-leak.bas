; riarexxlib-deleteport-leak (#530) - RXDeletePort never frees the
; 256-byte block of the message lists that RXCreatePort allocated with
; AllocVec (exec memory, not Blitz's: not freed at the end of the program
; either).  The program runs through inject (runwrap=, inject track: what
; the process leaves allocated); it creates and deletes its port.
; Correct: nothing is left allocated.
; @rwe riarexxlib
; @pass runwrap=@INJECT@|track
DEFTYPE .l
p=RXCreatePort("BUGORIG")
NPrint "created ",p<>0
RXDeletePort
End
