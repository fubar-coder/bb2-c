; compiler-usepath-overflow (#31) - dousepath copies the path of UsePath
; into usedpath (128 bytes) and appends the end byte 2 without a length
; check: a path of 128 or more characters runs past the buffer. In the
; original dousepath's own code follows usedpath, so the overflow
; overwrites its first instruction and the next UsePath runs the damaged
; code (the session times out). The port has other data behind usedpath
; and compiles the program - the verdict is DIFFERS. A line of TED has at
; most 128 characters, so the path comes from the macro p, which repeats
; its parameter of 65 characters: 130.
; Correct: a path that does not fit is refused, with "Path too Long" at
; the first UsePath.
Macro p
`1`1
End Macro
UsePath !p{qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq}
UsePath q
NPrint 1
End
