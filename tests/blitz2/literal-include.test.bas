; literal-include - an Include of a tokenised file with literal characters
; (spec compiler/literal-characters, #1018): literal-include.inc, tokenised by
; tedsrc tok, holds i$="ßé" (two literal characters) and a comment with
; one; the build stages it as CORPUS:src/literal-include.inc
; (literal-include.test.toml).
; Correct: the codes 223 and 233 and the length 2, with the original too.
!TestCase{include_file}
  Include "CORPUS:src/literal-include.inc"
  NPrint Asc(i$)," ",Asc(Right$(i$,1))," ",Len(i$)
!EndTestCase
