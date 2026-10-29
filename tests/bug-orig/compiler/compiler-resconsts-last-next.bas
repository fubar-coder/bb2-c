; compiler-resconsts-last-next (#34) - resconsts sets the next of every
; constant written to the resident to the offset behind it unless it was 0:
; the last constant is followed by the label .finish, so its next points
; past the constants, to the end of the hunk. fixconsts leaves it as loaded,
; and the compile's freelabels (from lastrescon) follows it and frees what
; it finds there. One constant: the hunk ends right behind it, and with
; inject pad behind every allocation (also LoadSeg's) come zeros: the
; FreeMem of a "label" at the hunk's end is reported, not done.
; Correct: no FreeMem inside the resident (next of the last constant 0).
; @pass wrap=@INJECT@|pad
; @pass file=r.bb2|@TESTDATA@/compiler-resconsts-last-next.res
; @pass jobline=resident=r|CORPUS:r.bb2|CORPUS:out/r.res
NPrint #width
NPrint "done"
End
