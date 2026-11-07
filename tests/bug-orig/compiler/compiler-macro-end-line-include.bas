; compiler-macro-end-line-include (#968) - the include case of
; compiler-macro-end-line: a macro whose last body line ends in End.  In an
; include the bytes behind the stored 0 are the next line, and readinc
; hands the same line out a second time: it is End Macro, the macro ends on
; the glued copy, the second copy is compiled as a statement and doend has
; no case for Macro: "Syntax Error" at the Include line.
; Correct (AmiBlitz3): the macro is defined, the program prints x.
; @fixed #968
; @pass file=src/inc.bb2|@TESTDATA@/compiler-macro-end-line-include.inc
Include "CORPUS:src/inc.bb2"
!m
End
