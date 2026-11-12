; An Include of a tokenised file with literal characters (spec
; compiler/literal-characters): literal-include.inc, tokenised by tedsrc
; tok, holds i$="ßé" (two literal characters) and a comment with one
Include "CORPUS:src/literal-include.inc"
NPrint Asc(i$)," ",Asc(Right$(i$,1))," ",Len(i$)
End
