; fileiolib-closefile-fields-stale (#140) - CloseFile frees the record
; fields of the file but leaves the pointer to them (+12 of the File
; object) and the record length (+4): the object points to freed memory
; until the next open, and a Fields in between adds to both.
; Here a file gets Fields with a long and a word (6 bytes) and is closed;
; printed are the record length and whether the field pointer is set,
; then the record length after a Fields with a byte on the closed file.
; Correct: after CloseFile the record length is 0 and no fields are set;
; the Fields afterwards gives a record of 1 byte.  The original keeps 6
; and the pointer, and the Fields makes it 7.
; @fixed #140
; @configs clib
f$="RAM:fileiolib-closefile-fields-stale.dat"
a.l=1 : b.w=2 : c.b=3
If OpenFile(0,f$)
  Fields 0,a,b
  o.l=Addr File(0)
  NPrint "open reclen ",Peek.l(o+4)," fields ",Peek.l(o+12)<>0
  CloseFile 0
  NPrint "closed reclen ",Peek.l(o+4)," fields ",Peek.l(o+12)<>0
  Fields 0,c
  NPrint "fields again reclen ",Peek.l(o+4)
EndIf
KillFile f$
End
