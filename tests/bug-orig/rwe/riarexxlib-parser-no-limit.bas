; riarexxlib-parser-no-limit (#537) - with RXTokenise on the command is
; copied into a 256-byte buffer (tempstorage) without a limit; in the
; original it lies in the code hunk directly in front of _nextparsearg,
; which runs right afterwards.  The command is the word "aaa", 253 spaces
; and the four bytes of CLR.l d0 : RTS: they land on the first
; instruction of _nextparsearg, which then returns "no word" at once
; (the message has 0 arguments instead of 1 - nothing else breaks, so
; the program goes on).  The C build has stringbuffer behind tempstorage:
; the four bytes become its first characters, and the word "aaa" parsed
; into it is read once more from there (2 arguments).
; Correct: the message arrives with the one argument "aaa".
; @rwe riarexxlib
; @pass timeout=30
DEFTYPE .l
p=RXCreatePort("BUGORIG")
RXTokenise On
RXRunCommand "BUGORIG","aaa"+String$(" ",253)+Chr$(66)+Chr$(128)+"Nu"
m=RXReadPort
NPrint "received ",m<>0
If m
  NPrint "one argument ",RXNumArgs=1
  RXReplyMsg 0
EndIf
End
