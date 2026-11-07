; compiler-makeiname-namebuff (#325) - makeiname copies the path of
; IncDir and then the file name of Include, XInclude or IncBin into
; namebuff (128 bytes) without a length check: together more than 127
; characters run past it, into namebuff2, from 264 characters into
; optstuff and then into the compiler's code. Here the path has 90
; characters and the name 60, together 150: the original writes 23 bytes
; into namebuff2 and reports "Can't open Include" (no such file).
; Correct: a name that does not fit is refused before it is copied, with
; "Path too Long" at the Include line.
IncDir "CORPUS:pppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp/"
Include "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn.bb2"
NPrint 1
End
