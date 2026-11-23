; riarexxlib-args-no-limit-16 (#527) - a RexxMsg has 16 argument slots,
; but the tokenizer stores one argstring per word without a count: the
; 17th and 18th word go into rm_PassPort and rm_CommAddr.  The program
; sends a command of 18 words to its own port and receives it;
; RXNumArgs counts the slots up to the first 0.
; Correct: at most 16 arguments, the 16th the rest of the command from
; its 16th word on (user decision W4 Q3 (b)), rm_PassPort stays 0.
DEFTYPE .l

!TestCase{eighteen_words}
  p=RXCreatePort("BUGORIG")
  RXTokenise On
  RXRunCommand "BUGORIG","1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18"
  m=RXReadPort
  !Require{m<>0,"message_received"}
  !CheckEq{RXNumArgs,16,"args"}
  pp=Peek.l(m+$68)<>0
  !CheckEq{pp,0,"passport_set"}
  a$=RXGetArg(16)
  !CheckStrEq{a$,"16 17 18","arg_16"}
!EndTestCase
