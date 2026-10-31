; compiler-incdir-overflow (#16) - doincdir copies the path of IncDir into
; the 96 byte buffer incdir without a length check: 97 characters and the
; 0 run 2 bytes past it.  The original keeps dirmode behind incdir: the
; program compiles, but crashes when it runs.  The port has other data
; behind incdir (BSS: comdata, the editor's table) and the compiler itself
; crashes - the verdict is DIFFERS until the data layout matches.
; Correct (fix of the issue): an error at the IncDir line.
; @pass timeout=30
IncDir "ppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp"
x=5
NPrint x
End
