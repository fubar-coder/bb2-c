; t75_idcmperr - windowslib error check idcmpchk: DefaultIDCMP 0
; (no window open: a runtime error must not leave windows behind)
NPrint "before"
DefaultIDCMP 0
NPrint "after"
End
