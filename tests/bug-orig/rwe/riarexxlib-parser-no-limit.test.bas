; riarexxlib-parser-no-limit (#537) - with RXTokenise on the command is
; copied into a 256-byte buffer (tempstorage) without a limit; in the
; original it lies in the code hunk directly in front of _nextparsearg,
; which runs right afterwards.  The command is the word "aaa", 253 spaces
; and the four bytes of CLR.l d0 : RTS: they land on the first
; instruction of _nextparsearg, which then returns "no word" at once
; (the message has 0 arguments instead of 1 - nothing else breaks, so
; the program goes on).  The C build has stringbuffer behind tempstorage:
; the four bytes become its first characters, and the word "aaa" parsed
; into it is read once more from there.
; Correct: the message arrives with the two words of the command, "aaa"
; and the four bytes.
DEFTYPE .l

!TestCase{long_command}
  p=RXCreatePort("BUGORIG")
  RXTokenise On
  RXRunCommand "BUGORIG","aaa"+String$(" ",253)+Chr$(66)+Chr$(128)+"Nu"
  m=RXReadPort
  received=(m<>0)
  !CheckEq{received,-1,"received"}
  !Require{m<>0,"message_received"}
  !CheckEq{RXNumArgs,2,"args"}
  a$=RXGetArg(1)
  !CheckStrEq{a$,"aaa","arg_1"}
  !CheckEq{Len(RXGetArg(2)),4,"arg_2_length"}
  RXReplyMsg 0
!EndTestCase
