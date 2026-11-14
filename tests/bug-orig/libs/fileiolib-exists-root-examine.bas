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
; @fixed #1026
; @configs clib
; @pass runwrap=@INJECT@|examinefail=exists-root-examine.bad
f$="RAM:exists-root-examine.dat"
g$="RAM:exists-root-examine.bad"
Dim m.b(99)
If WriteFile(0,f$) Then WriteMem 0,&m(0),100 : CloseFile 0
If WriteFile(0,g$) Then WriteMem 0,&m(0),50 : CloseFile 0
NPrint "drawer ",Exists("SYS:C")
NPrint "root SYS: ",Exists("SYS:")
NPrint "root RAM: ",Exists("RAM:")
NPrint "file ",Exists(f$)
NPrint "examine failed ",Exists(g$)
KillFile f$ : KillFile g$
End
