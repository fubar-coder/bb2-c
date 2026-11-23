; riarexxlib-nextarg-empty (#534) - RXNextArg advances to the next
; argument only when the one it returned is not empty (it tests the
; length, not the argument slot): at an empty argument it returns "" for
; ever.  The program sends "a b c" to its own port (three arguments),
; receives the message and makes argument 2 empty (its first byte 0: the
; tokenizer cannot make an empty argument), then calls RXNextArg three
; times.
; Correct: a, "", c.
DEFTYPE .l

!TestCase{empty_second}
  p=RXCreatePort("BUGORIG")
  RXTokenise On
  RXRunCommand "BUGORIG","a b c"
  m=RXReadPort
  !Require{m<>0,"message_received"}
  Poke.b Peek.l(m+$2C),0
  !CheckEq{RXNumArgs,3,"args"}
  a$=RXNextArg
  !CheckStrEq{a$,"a","next_1"}
  a$=RXNextArg
  !CheckStrEq{a$,"","next_2"}
  a$=RXNextArg
  !CheckStrEq{a$,"c","next_3"}
  RXReplyMsg 0
!EndTestCase
