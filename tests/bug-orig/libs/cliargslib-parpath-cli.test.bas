; cliargslib-parpath-cli (#107) - ParPath$ is for Workbench arguments; a
; CLI argument has no lock (0). Without runtime error checking the
; original hands lock 0 to NameFromLock and returns the name of the boot
; volume (+ "/" + the argument with type 1) instead of an empty string.
; The program is started from the CLI with one argument (runargs=).
; Correct: an empty string for both types.
DEFTYPE .l

!TestCase{parpath}
  n=NumPars
  a$=Par$(1)
  p0$=ParPath$(1,0)
  p1$=ParPath$(1,1)
  !CheckEq{n,1,"numpars"}
  !CheckStrEq{a$,"datei.txt","par1"}
  !CheckStrEq{p0$,"","parpath_0"}
  !CheckStrEq{p1$,"","parpath_1"}
!EndTestCase
