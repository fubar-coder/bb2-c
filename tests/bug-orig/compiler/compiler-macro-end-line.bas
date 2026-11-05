; compiler-macro-end-line (#968) - domacro reads the character after the
; token End with getchar3 (to recognise End Macro) and stores anything else
; unchecked.  When the line ends right after End, getcharb has stepped past
; the line's NUL and returns 0: the 0 is stored (80 02 00 00) and the
; collector goes on behind the line's terminator.  The stored 0 ends the
; expanded line: the End If is lost and the call fails with "If Without End
; If" (the include case is compiler-macro-end-line-include).
; Correct (AmiBlitz3): the macro body is complete, the program prints
; after and done.
Macro m
  If `1
    End
  End If
  NPrint "after"
End Macro
!m{0}
NPrint "done"
End
