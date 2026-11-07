; compiler-incdir-overflow (#16) - doincdir copies the path of IncDir into
; the 96 byte buffer incdir without a length check: 97 characters and the
; 0 run 2 bytes past it.  The original keeps dirmode behind incdir: the
; program compiles, but crashes when it runs.
; Correct: a path longer than 95 characters is refused, with "Path too
; Long" at the IncDir line.
; @fixed #16
; @pass timeout=30
IncDir "ppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp"
x=5
NPrint x
End
