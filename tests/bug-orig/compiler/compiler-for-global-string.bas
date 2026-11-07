; compiler-for-global-string (#39) - after a string assignment dolet2 returns
; varmode (0 for a global variable) as the type of the variable, so dofor's
; type check lets a global string through as a For variable.
; Correct: a string For variable is refused ("Bad Type for For...Next"), as
; it is for a local one.
; @fixed #39
For a$="a" To 10
Next
NPrint "done"
End
