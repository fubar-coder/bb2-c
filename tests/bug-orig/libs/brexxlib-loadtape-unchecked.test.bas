; brexxlib-loadtape-unchecked (#101) - LoadTape reads the 8 byte start
; time without checking the result: any file becomes a tape, a file
; shorter than 8 bytes gives a start time from its bytes.
; Here: a file of 5 bytes ("ABCDE").
; Correct: the read of the start time fails, the tape stays empty (no
; events, start time 0).  The original takes "ABCDE" as the start time.
DEFTYPE .l

!TestCase{short_file}
  ok=WriteFile(0,"CORPUS:bl101.tape")
  !Require{ok,"file_written"}
  FileOutput 0 : Print "ABCDE" : CloseFile 0
  DefaultOutput
  LoadTape 0,"CORPUS:bl101.tape"
  KillFile "CORPUS:bl101.tape"
  a=Addr Tape(0)
  !CheckEq{Peek.l(a),0,"events"}
  !CheckEq{Peek.l(a+4),0,"start_time_high"}
  !CheckEq{Peek.l(a+8),0,"start_time_low"}
!EndTestCase
