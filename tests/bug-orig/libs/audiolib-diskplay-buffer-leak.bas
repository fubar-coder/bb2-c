; audiolib-diskplay-buffer-leak (#950) - DiskPlay allocates its two buffers
; (2 * DiskBuffer bytes of chip memory, memlib globalloc) before it opens
; the file; when doopensound fails (err1 "Illegal Sound file" if the open
; fails, err2/err3 for a missing VHDR or a read error) it goes into the
; runtime error without freeing them.  memlib frees them at the program's
; end, but a program that catches the error (SetErr) and goes on has lost
; them.  DiskBuffer 20000 (40000 bytes); the file does not exist.
; Correct: after the error the chip memory is as before DiskPlay.
; @configs clib
DEFTYPE .l
DiskBuffer 20000
NPrint "start"
SetErr
  a=AvailMem_(2)
  NPrint "error"
  If b-a>=40000
    NPrint "chip buffer kept"
  Else
    NPrint "chip buffer freed"
  EndIf
  End
End SetErr
b=AvailMem_(2)
DiskPlay "CORPUS:missing.8svx",1
NPrint "played"
End
