; fileiolib-closefile-fields-stale (#140) - CloseFile frees the record
; fields of the file but leaves the pointer to them (+12 of the File
; object) and the record length (+4): the object points to freed memory
; until the next open, and a Fields in between adds to both.
; Here a file gets Fields with a long and a word (6 bytes) and is closed;
; checked are the record length and whether the field pointer is set,
; then the record length after a Fields with a byte on the closed file.
; Correct: after CloseFile the record length is 0 and no fields are set;
; the Fields afterwards gives a record of 1 byte.  The original keeps 6
; and the pointer, and the Fields makes it 7.
DEFTYPE .l

!TestCase{fields_stale}
  f$="CORPUS:fileiolib-closefile-fields-stale.dat"
  a=1 : b.w=2 : c.b=3
  r=OpenFile(0,f$)
  !Require{r,"opened"}
  Fields 0,a,b
  o=Addr File(0)
  !CheckEq{Peek.l(o+4),6,"open_reclen"}
  !Check{Peek.l(o+12)<>0,"open_fields"}
  CloseFile 0
  !CheckEq{Peek.l(o+4),0,"closed_reclen"}
  !Check{Peek.l(o+12)=0,"closed_fields"}
  Fields 0,c
  !CheckEq{Peek.l(o+4),1,"again_reclen"}
  KillFile f$
!EndTestCase
