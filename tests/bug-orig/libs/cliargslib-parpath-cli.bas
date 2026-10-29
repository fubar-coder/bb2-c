; cliargslib-parpath-cli (#107) - ParPath$ is for Workbench arguments; a
; CLI argument has no lock (0). Without runtime error checking the
; original hands lock 0 to NameFromLock and returns the name of the boot
; volume (+ "/" + the argument with type 1) instead of an empty string.
; The program is started from the CLI with one argument (runargs=).
; Correct: an empty string for both types.
; @configs clib
; @pass runargs=datei.txt
NPrint "numpars ",NumPars," ",Par$(1)
NPrint "parpath0 [",ParPath$(1,0),"]"
NPrint "parpath1 [",ParPath$(1,1),"]"
End
