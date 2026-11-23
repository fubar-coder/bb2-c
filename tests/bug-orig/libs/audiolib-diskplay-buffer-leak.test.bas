; audiolib-diskplay-buffer-leak (#950) - DiskPlay allocates its two buffers
; (2 * DiskBuffer bytes of chip memory, memlib globalloc) before it opens
; the file; when doopensound fails (err1 "Illegal Sound file" if the open
; fails, err2/err3 for a missing VHDR or a read error) it goes into the
; runtime error without freeing them.  memlib frees them at the program's
; end, but a program that catches the error (SetErr) and goes on has lost
; them.
; Here: DiskBuffer 20000 (40000 bytes); DiskPlay of a file that does not
; exist; the handler of SetErr compares the free chip memory with that
; before DiskPlay and ends the program.
; Correct: after the error the chip memory is as before DiskPlay.
DEFTYPE .l

!TestCase{missing_file}
  DiskBuffer 20000
  SetErr
    a=AvailMem_(2)
    kept=0
    If b-a>=40000 Then kept=1
    !CheckEq{kept,0,"chip_buffer_kept"}
    End
  End SetErr
  b=AvailMem_(2)
  DiskPlay "CORPUS:missing.8svx",1
  !Check{0,"not_refused"}
!EndTestCase
