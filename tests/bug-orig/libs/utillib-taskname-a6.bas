; utillib-taskname-a6 (#1060) - TaskName$ of the own task (0) loads exec's
; base into a6 (FindTask) and does not restore it.  The compiler tracks a6
; within a statement and does not reload a library base it believes is
; still there: SystemDate (a function that takes the base of dos.library
; in a6) is called twice around TaskName$ in one expression, so the second
; call goes through exec's base.
; Correct: the sum of the two dates and the length of the name is what the
; parts give one by one.
; @fixed #1060
; @configs clib
DEFTYPE .l
c=SystemDate
n=Len(TaskName$(0))
a=SystemDate+Len(TaskName$(0))+SystemDate
NPrint "sum right ",a=c+n+c
End
