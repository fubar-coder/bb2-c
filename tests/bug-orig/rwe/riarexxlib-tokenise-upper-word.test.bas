; riarexxlib-tokenise-upper-word (#547) - RXTokenise takes a word, but
; stores the whole register (MOVE.l d0) and tests the long: "off" whose
; register has a set upper word stays on.  The program passes the long
; $10000, whose word is 0 (off), sends "a b c" to its own port and counts
; the arguments of the message it receives.
; Correct: tokenising is off, the command is one argument - 1.
DEFTYPE .l

!TestCase{upper_word}
  p=RXCreatePort("BUGORIG")
  x=$10000
  RXTokenise x
  RXRunCommand "BUGORIG","a b c"
  m=RXReadPort
  !Require{m<>0,"message_received"}
  !CheckEq{RXNumArgs,1,"args"}
  RXReplyMsg 0
!EndTestCase
