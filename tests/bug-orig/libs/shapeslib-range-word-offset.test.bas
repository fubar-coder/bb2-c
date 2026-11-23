; shapeslib-range-word-offset (#999) - LoadShapes Shape#,File$ and
; DecodeShapes Shape#,Memory load from the given shape up to the last
; shape of the object table, computed as table + (maximum - 1) << 5 with
; a word add: from a maximum of 1025 on the last shape lies before the
; table, and only the first shape is loaded.
; Here: the shape maximum is raised to 1100; three shapes of 8, 16 and 24
; pixels width are saved, freed and loaded again with LoadShapes 0,File$,
; then freed and decoded from the file in memory with DecodeShapes 0,Mem
; (shapes 0 to 2 lie at the start of the table: the compiler addresses
; them alike with and without #939).
; Correct: all three shapes are back after either command.
DEFTYPE .l
Function.s widths{}
  a$=""
  For s=0 To 2 : a$=a$+Str$(Peek.w(Addr Shape(s)))+" " : Next
  Function Return a$
End Function
Statement freeall{}
  For s=0 To 2 : Free Shape s : Next
End Statement

!TestCase{range_word}
  BitMap 0,32,8,2
  Use BitMap 0
  Boxf 0,0,31,7,3
  GetaShape 0,0,0,8,8
  GetaShape 1,0,0,16,8
  GetaShape 2,0,0,24,8
  !CheckStrEq{widths{},"8 16 24 ","made"}
  SaveShapes 0,2,"CORPUS:bo999.shp"
  freeall{}
  !CheckStrEq{widths{},"0 0 0 ","freed"}
  LoadShapes 0,"CORPUS:bo999.shp"
  !CheckStrEq{widths{},"8 16 24 ","loadshapes"}
  freeall{}
  ok=ReadFile(0,"CORPUS:bo999.shp")
  !Require{ok,"shapes_file_read"}
  n=Lof(0) : m=AllocMem(n,2)
  ReadMem 0,m,n
  CloseFile 0
  DecodeShapes 0,m
  !CheckStrEq{widths{},"8 16 24 ","decodeshapes"}
  KillFile "CORPUS:bo999.shp"
!EndTestCase
