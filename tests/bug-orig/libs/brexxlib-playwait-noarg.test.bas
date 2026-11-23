; brexxlib-playwait-noarg (#99) - the help of PlayWait has no argument
; ("wait for the playback to finish"), but the command needs a Tape#,
; and it plays only while a playback runs.
; Here: PlayBack of a tape (LoadTape, testdata) with two pointer events
; 1 and 2 s after its start, then PlayWait without an argument.
; Correct: PlayWait waits until the playback has finished: XStatus is
; then 0.  The original does not compile it.
DEFTYPE .l

!TestCase{wait_without_argument}
  LoadTape 0,"CORPUS:tape"
  PlayBack 0
  PlayWait
  !CheckEq{XStatus,0,"status_finished"}
!EndTestCase
