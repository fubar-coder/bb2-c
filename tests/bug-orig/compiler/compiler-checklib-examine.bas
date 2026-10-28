; compiler-checklib-examine (#27) - checklib takes the length of
; blitz2:acidlibs from the FileInfoBlock without checking Examine: when
; Examine fails (inject), the length of blitz2:deflibs examined just
; before is used, the first 55592 bytes of acidlibs are loaded and the
; libraries in them (animlib among them) are linked - with garbage behind
; the cut.
; Correct: a failed Examine counts as "not found" (checklib returns 0):
; acidlibs is not loaded, the animlib token is unknown - the same as
; without blitz2:acidlibs.
; @pass wrap=@INJECT@|examinefail=blitz2:acidlibs
InitAnim 0
End
