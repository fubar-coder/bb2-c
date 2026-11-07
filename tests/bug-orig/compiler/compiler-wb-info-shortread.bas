; compiler-wb-info-shortread (#19) - started from the Workbench, the
; startup code of blitz2 reads blitz2.info for the stack size; if Read
; returns fewer than $4E bytes, it leaves the file open.  blitz2.info is
; replaced by 5 bytes (generated); wbrun starts blitz2 like the Workbench,
; with the job list as project icon, and checks after the end whether
; blitz2.info is still in use.
; Correct: the file is closed.
; @fixed #19
; @pass file=blitz2/blitz2.info|@TESTDATA@/compiler-wb-info-shortread.info
; @pass wrap=@WBRUN@|inuse=blitz2:blitz2.info
NPrint "hello"
End
