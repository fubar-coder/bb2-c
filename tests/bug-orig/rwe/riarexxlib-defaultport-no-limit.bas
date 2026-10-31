; riarexxlib-defaultport-no-limit (#529) - RXDefaultPort copies the name
; without a limit into its 65 bytes; behind them lie rexxbase and the
; other variables.  A name of 70 characters overwrites rexxbase with
; "PPPP": the next CreateArgString (the message of RXRunCommand, here to
; the program's own port) jumps through it - a crash (the session ends
; in a timeout).
; Correct: the name is refused or cut, the message is sent and arrives.
; @rwe riarexxlib
; @pass timeout=30
DEFTYPE .l
p=RXCreatePort("BUGORIG")
RXTokenise On
RXDefaultPort String$("P",70)
NPrint "set"
RXRunCommand "BUGORIG","hello"
m=RXReadPort
NPrint "received ",m<>0
End
