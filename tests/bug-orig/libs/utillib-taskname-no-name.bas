; utillib-taskname-no-name (#297) - TaskName$(task) copies the name from
; ln_Name (offset 10) of the task without a test for 0: a task without a
; name copies from address 0 up to the first 0 byte, without a limit,
; into the string work area.
; Here a zeroed block stands for a task without a name; the text "ABC"
; with its 0 is put at address 0 first (the long there is restored at
; once), so the original's result is known: "ABC".
; Correct: a task without a name has the empty name: length 0.
; @fixed #297
; @configs clib
DEFTYPE .l
Dim t.l(24)
o=Peek.l(0)
Poke.l 0,$41424300
a$=TaskName$(&t(0))
Poke.l 0,o
NPrint "len ",Len(a$)," [",a$,"]"
NPrint "own ",Len(TaskName$(0))>0
End
