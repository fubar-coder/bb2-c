; riarexxlib-createport-name-exists (#1096) - RXCreatePort does not look
; for a public port of the same name: it adds a second port of the name,
; although the manual names exactly that as the usual failure ("0 if it
; fails. Failure is usually due to a public message port existing with
; the same name").  The program makes a public port BUGORIG with exec
; itself and then calls RXCreatePort("BUGORIG").
; Correct: RXCreatePort returns 0.
; @fixed #1096
; @rwe riarexxlib
DEFTYPE .l
n$="BUGORIG"
mp=CreateMsgPort_
Poke.l mp+10,&n$
AddPort_ mp
p=RXCreatePort("BUGORIG")
NPrint "refused ",p=0
If p Then RXDeletePort
RemPort_ mp
DeleteMsgPort_ mp
End
