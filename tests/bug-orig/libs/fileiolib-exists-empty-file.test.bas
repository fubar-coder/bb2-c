; fileiolib-exists-empty-file (#143, won't fix) - Exists returns the length
; of the file (the manual: "Exists actually returns the length of the
; file ... If 0 the file either does not exist or is empty"), so an
; existing empty file gives 0 like a missing one.  Kept: programs use the
; value as the length (AllocMem, ReadMem, LoadBank), and a new value for
; an empty file would break them; "is it there" is ReadFile.
; Here an empty file is written and closed, then Exists of it and of a
; missing file.
; Correct: the empty file is told apart from the missing one.  The
; original (and the port) give 0 for both.
DEFTYPE .l

!TestCase{empty_file}
  e$="CORPUS:fileiolib-exists-empty-file.dat"
  m$="CORPUS:fileiolib-exists-empty-file.none"
  KillFile m$
  r=WriteFile(0,e$)
  !Require{r,"written"}
  CloseFile 0
  ex=Exists(e$) : mi=Exists(m$)
  !Check{ex<>0,"empty_there"}
  !Check{mi=0,"missing_missing"}
  r=ReadFile(0,e$)
  !Check{r<>0,"readfile_ok"}
  If r Then CloseFile 0
  KillFile e$
!EndTestCase
