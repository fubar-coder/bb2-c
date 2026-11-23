; riseriallib-sendstring (#801) - SerialSendString sends the low byte of
; the string's address (with the stop bit) once for every long it reads
; from the a2 stack, down to one with a low byte 0, instead of the string.
; The program opens the port and sends the request for "HELLO" to the
; remote end (serial=on, tests/corpus/runner/serial.h) with
; SerialSendString, then reads the reply with SerialRead (each empty read
; followed by a VWait, at most 100).
; Correct: SerialSendString sends the characters of the string: the reply
; comes.
DEFTYPE .l

!TestCase{sendstring}
  SerialOpen 19200
  q$=Chr$(1)+"riseriallib-sendstring"+Chr$(2)+Chr$(5)+"B"
  SerialSendString q$
  r$="" : n=0
  While Len(r$)<5 AND n<100
    c=SerialRead
    If c=-1 Then VWait : n+1 Else r$+Chr$(c AND $FF)
  Wend
  SerialWrite 4 : VWait 10
  !CheckStrEq{r$,"HELLO","reply"}
!EndTestCase
