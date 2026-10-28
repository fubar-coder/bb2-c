; t50_fileio - fileiolib: OpenFile/ReadFile/WriteFile/CloseFile/KillFile,
; sequential I/O with FileOutput/FileInput (Edit$, Inkey$), Exists,
; Lof/Loc/Eof, FileSeek, ReadMem/WriteMem, DosBuffLen, missing files
f$="RAM:t50a.txt"
KillFile f$
NPrint "exists missing ",Exists(f$)
NPrint "readfile missing ",ReadFile(0,f$)
NPrint "exists drawer ",Exists("SYS:C")
NPrint "writefile ",WriteFile(0,f$)
NPrint "empty lof ",Lof(0)," loc ",Loc(0)," eof ",Eof(0)
NPrint "exists empty ",Exists(f$)
FileOutput 0
Print "hello"
NPrint " world"
NPrint 12345
NPrint ""
NPrint "x"
Print "no newline"
DefaultOutput
NPrint "written lof ",Lof(0)," loc ",Loc(0)," eof ",Eof(0)
CloseFile 0
NPrint "exists ",Exists(f$)
CloseFile 0
NPrint "read ",ReadFile(1,f$)
NPrint "lof ",Lof(1)," loc ",Loc(1)," eof ",Eof(1)
FileInput 1
For i=1 To 6
  a$=Edit$(80)
  NPrint "line ",i," [",a$,"] loc ",Loc(1)," eof ",Eof(1)
Next
DefaultInput
FileSeek 1,2
NPrint "seek 2 loc ",Loc(1)," eof ",Eof(1)
FileInput 1
a$=Inkey$ : b$=Inkey$(3)
DefaultInput
NPrint "inkey [",a$,"] [",b$,"] loc ",Loc(1)
FileSeek 1,Lof(1)
NPrint "seek end loc ",Loc(1)," eof ",Eof(1)
FileInput 1
c$=Inkey$
DefaultInput
NPrint "inkey at end [",c$,"] ",Len(c$)
FileSeek 1,0
FileInput 1
a$=Edit$(3) : b$=Edit$(80)
DefaultInput
NPrint "edit 3 [",a$,"] [",b$,"] loc ",Loc(1)
CloseFile 1
; OpenFile on an existing file: position 0, overwrite in place
NPrint "open ",OpenFile(2,f$)
FileOutput 2 : Print "HE" : DefaultOutput
NPrint "loc ",Loc(2)," lof ",Lof(2)
FileSeek 2,Lof(2)
FileOutput 2 : NPrint "tail" : DefaultOutput
NPrint "loc ",Loc(2)," lof ",Lof(2)," eof ",Eof(2)
FileSeek 2,0
FileInput 2
For i=1 To 7
  a$=Edit$(80) : NPrint "[",a$,"]"
Next
DefaultInput
CloseFile 2
; ReadMem/WriteMem with a tiny dos buffer: direct and buffered paths
Dim m.b(99)
For i=0 To 99 : m(i)=i : Next
g$="RAM:t50b.dat"
DosBuffLen 8
NPrint "write ",WriteFile(3,g$)
WriteMem 3,&m(0),5
WriteMem 3,&m(5),3
WriteMem 3,&m(8),20
WriteMem 3,&m(28),0
WriteMem 3,&m(28),7
NPrint "loc ",Loc(3)," lof ",Lof(3)," eof ",Eof(3)
FileSeek 3,10
WriteMem 3,&m(90),2
NPrint "loc ",Loc(3)," lof ",Lof(3)
CloseFile 3
NPrint "size ",Exists(g$)
For i=0 To 99 : m(i)=0 : Next
NPrint "read ",ReadFile(3,g$)
ReadMem 3,&m(0),3
ReadMem 3,&m(3),6
NPrint "loc ",Loc(3)," eof ",Eof(3)
ReadMem 3,&m(9),10
ReadMem 3,&m(19),30
NPrint "loc ",Loc(3)," eof ",Eof(3)
s$="" : For i=0 To 39 : s$=s$+Str$(m(i))+" " : Next : NPrint s$
FileSeek 3,33
ReadMem 3,&m(50),4
NPrint "last ",m(50)," ",m(51)," loc ",Loc(3)," eof ",Eof(3)
CloseFile 3
DosBuffLen 1
NPrint "write1 ",WriteFile(4,g$)
FileOutput 4 : Print "abc" : NPrint "defgh" : DefaultOutput
NPrint "loc ",Loc(4)," lof ",Lof(4)
CloseFile 4
DosBuffLen 2048
NPrint "read1 ",ReadFile(4,g$)
FileInput 4 : a$=Edit$(80) : DefaultInput
NPrint "[",a$,"] eof ",Eof(4)
CloseFile 4
KillFile f$ : KillFile g$
NPrint "killed ",Exists(f$)," ",Exists(g$)
NPrint "readfile new only ",WriteFile(0,"RAM:t50c.dat")," ",ReadFile(1,"RAM:t50c.dat")
CloseFile 0 : CloseFile 1
KillFile "RAM:t50c.dat"
End
