; fileiolib-eof-loc-lof (#144) - Eof at the end of a file tries a seek one
; byte further outside the buffer; myseek stores the new position as the
; buffer's file position before the dos Seek, and when that Seek fails
; nothing puts it back: Loc and Lof are 1 too high afterwards (and the
; append idiom FileSeek f,Lof(f) asks for a position behind the end).
; Here Eof on a new empty file, then 3 bytes written (Loc/Lof); the file
; read back to its end and Eof (Loc/Lof); Eof after a FileSeek to the end
; of an OpenFile (Loc/Lof).
; Correct: positions count from 0, at the end Loc = Lof = the length, 3.
; The original gives 4.
DEFTYPE .l

!TestCase{loc_lof}
  f$="CORPUS:fileiolib-eof-loc-lof.dat"
  KillFile f$
  r=WriteFile(0,f$)
  !Require{r,"written"}
  e=Eof(0)
  !CheckEq{e,-1,"new_eof"}
  FileOutput 0 : Print "abc" : DefaultOutput
  l1=Loc(0) : l2=Lof(0)
  !CheckEq{l1,3,"written_loc"}
  !CheckEq{l2,3,"written_lof"}
  CloseFile 0
  !CheckEq{Exists(f$),3,"exists_after_write"}
  r=ReadFile(1,f$)
  !Require{r,"read_open"}
  FileInput 1 : a$=Inkey$(3) : DefaultInput
  e=Eof(1)
  l1=Loc(1) : l2=Lof(1)
  !CheckStrEq{a$,"abc","read_text"}
  !CheckEq{e,-1,"read_eof"}
  !CheckEq{l1,3,"read_loc"}
  !CheckEq{l2,3,"read_lof"}
  CloseFile 1
  r=OpenFile(2,f$)
  !Require{r,"open_open"}
  FileSeek 2,3
  e=Eof(2)
  l1=Loc(2) : l2=Lof(2)
  !CheckEq{e,-1,"open_eof"}
  !CheckEq{l1,3,"open_loc"}
  !CheckEq{l2,3,"open_lof"}
  CloseFile 2
  !CheckEq{Exists(f$),3,"exists_after_read"}
  KillFile f$
!EndTestCase
