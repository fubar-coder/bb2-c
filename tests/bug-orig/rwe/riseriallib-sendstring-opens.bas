; riseriallib-sendstring-opens (#793) - SerialSendString and
; SerialReadString test the open flag the wrong way round: they call
; SerialOpen's code only when the port is already open, so with the port
; closed nothing is sent.  The program sends the request for "HELLO" to
; the remote end (serial=on, tests/corpus/runner/serial.h) with
; SerialSendString without SerialOpen and reads the reply with SerialRead
; (each empty read followed by a VWait, at most 100).
; Correct: SerialSendString opens the port: the request goes out (with the
; fix of #801, the characters of the string), the reply comes.
; @rwe riseriallib
; @pass serial=on
DEFTYPE .l
q$=Chr$(1)+"riseriallib-sendstring-opens"+Chr$(2)+Chr$(5)+"B"
SerialSendString q$
r$="" : n=0
While Len(r$)<5 AND n<100
  c=SerialRead
  If c=-1 Then VWait : n+1 Else r$+Chr$(c AND $FF)
Wend
NPrint "reply: ",r$
SerialWrite 4 : VWait 10
End
