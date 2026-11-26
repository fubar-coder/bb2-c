; stop-off - a plain Stop without runtime error checking is still refused,
; as in Blitz Basic 2 (the original gives the same error, no deviation)
; Correct: the compile ends with "Debugger must be enabled for STOP" at the
; line of the Stop, nothing runs.
!TestCase{stop_off}
  NPrint "start"
  Stop
!EndTestCase
