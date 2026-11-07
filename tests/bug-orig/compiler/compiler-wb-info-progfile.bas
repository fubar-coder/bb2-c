; compiler-wb-info-progfile (#20) - started from the Workbench, the
; startup code of blitz2 reads blitz2.info into progfile, the name of the
; program the editor loads, and clears it only after a successful read:
; without a project icon a short blitz2.info leaves its bytes there as the
; name.  blitz2.info is replaced by the 12 bytes "CORPUS:jobs" and a 0
; (generated, shorter than a DiskObject); wbrun starts blitz2 like the
; Workbench without a project icon (noprojects).  The editor (tedstub)
; then loads CORPUS:jobs, the job list, and compiles.
; Correct: progfile is empty, the editor loads nothing, no job runs.
; @fixed #20
; @pass file=blitz2/blitz2.info|@TESTDATA@/compiler-wb-info-progfile.info
; @pass wrap=@WBRUN@|noprojects
NPrint "hello"
End
