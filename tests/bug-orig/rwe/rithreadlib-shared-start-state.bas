; rithreadlib-shared-start-state (#838) - CreateThread keeps the code
; address, the registers and the memory of the new process in single
; variables, which the process reads only when it first runs;
; CreateNewProc returns before that (the new process has the program's
; priority, the program goes on).  Two CreateThread in a row start both
; threads with the second one's code: codea never runs, codeb twice.
; The program waits (at most 10 s) until no thread is left and prints
; how often each code ran.
; Correct: each thread runs its own code once: a 1, b 1.
; @rwe rithreadlib
DEFTYPE .l ga,gb
ga=0 : gb=0
CreateThread "bugorig-one",?codea
CreateThread "bugorig-two",?codeb
n=0
While ActiveThreads>0 AND n<200
  Delay_ 5
  n=n+1
Wend
NPrint "a ",ga
NPrint "b ",gb
End
codea:
ga=ga+1
RTS
codeb:
gb=gb+1
RTS
