; trackerlib-loadmodule-leak (#296) - LoadModule gets a FileInfoBlock
; (260 bytes, memlib globalloc) and a lock on the name; when Examine fails
; or the name is a drawer it goes into the runtime error "Error Accessing
; Module File" without freeing the FileInfoBlock and without UnLock: a
; program that catches the error (SetErr) and goes on has lost the
; memory, and the drawer stays locked.
; Here: LoadModule of a drawer (the configuration makes it) as a module;
; the handler of SetErr compares the free memory with that before
; LoadModule, then asks for an exclusive lock on the drawer, which fails
; while the shared lock of LoadModule is open (object in use), and ends
; the program.
; Correct: after the error the memory is as before LoadModule, and the
; exclusive lock works (no lock is left on the drawer).
DEFTYPE .l

!TestCase{drawer_as_module}
  SetErr
    a=AvailMem_(1)
    kept=0
    If b-a>=260 Then kept=1
    l=Lock_("CORPUS:t296",-1)
    got=0
    If l Then got=1 : UnLock_ l
    !CheckEq{kept,0,"fileinfoblock_kept"}
    !CheckEq{got,1,"exclusive_lock"}
    End
  End SetErr
  b=AvailMem_(1)
  LoadModule 0,"CORPUS:t296"
  !Check{0,"not_refused"}
!EndTestCase
