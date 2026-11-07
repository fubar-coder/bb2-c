; compiler-makename-overflow (#29) - makename4 copies an identifier into
; namebuff (128 bytes) without a length check: a name of 128 characters
; writes its 0 past the buffer, longer ones run on into the data behind it.
; A TED line has at most 128 characters: the names come from a macro
; (`1`1`2: 60+60+7 = 127 characters, then 60+60+8 = 128).
; Correct: a name of up to 127 characters is accepted, a longer one is a
; compile error.
; @fixed #29
Macro s
`1`1`2=1
NPrint `1`1`2
End Macro
!s{aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,aaaaaaa}
!s{bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb,bbbbbbbb}
End
