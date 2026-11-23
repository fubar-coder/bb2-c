; fileiolib-exists-root-examine (#1026) - Exists gives -1 only for
; fib_DirEntryType 2 (a drawer): a volume's root (type 1) returns its
; fib_Size, 0 on the usual file systems, which Exists otherwise means "not
; there"; and the result of Examine is not checked, so a failing Examine
; returns what the FileInfoBlock (memlib memory, not cleared) holds.
; Here Exists of a drawer, of the roots SYS: and RAM:, of a file of 100
; bytes, and then of another file whose Examine fails (inject
; examinefail): its FileInfoBlock is the block the previous Exists used.
; Correct: -1 for the drawer and the roots, 100 for the file, 0 when
; Examine fails.  The original gives the 100 of the previous file for the
; failed Examine.  (The roots of the FFS and RAM: volumes of the emulator
; report type 2, so they show -1 in the original too; other handlers
; report type 1.)
DEFTYPE .l

!TestCase{examine}
  f$="CORPUS:exists-root-examine.dat"
  g$="CORPUS:exists-root-examine.bad"
  Dim m.b(99)
  r=WriteFile(0,f$)
  !Require{r,"file_written"}
  WriteMem 0,&m(0),100 : CloseFile 0
  r=WriteFile(0,g$)
  !Require{r,"bad_written"}
  WriteMem 0,&m(0),50 : CloseFile 0
  !CheckEq{Exists("SYS:C"),-1,"drawer"}
  !CheckEq{Exists("SYS:"),-1,"root_sys"}
  !CheckEq{Exists("RAM:"),-1,"root_ram"}
  !CheckEq{Exists(f$),100,"file"}
  !CheckEq{Exists(g$),0,"examine_failed"}
  KillFile f$ : KillFile g$
!EndTestCase
