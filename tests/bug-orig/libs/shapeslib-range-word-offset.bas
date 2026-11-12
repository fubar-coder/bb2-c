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
; @fixed #999
; @pass jobline=max=154|1100
DEFTYPE .l
Statement widths{t$}
  Print t$
  For s=0 To 2 : Print " ",Peek.w(Addr Shape(s)) : Next
  NPrint ""
End Statement
Statement freeall{}
  For s=0 To 2 : Free Shape s : Next
End Statement
BitMap 0,32,8,2
Use BitMap 0
Boxf 0,0,31,7,3
GetaShape 0,0,0,8,8
GetaShape 1,0,0,16,8
GetaShape 2,0,0,24,8
widths{"made"}
SaveShapes 0,2,"RAM:bo999.shp"
freeall{}
widths{"freed"}
LoadShapes 0,"RAM:bo999.shp"
widths{"loadshapes"}
freeall{}
If ReadFile(0,"RAM:bo999.shp")
  n=Lof(0) : m=AllocMem(n,2)
  ReadMem 0,m,n
  CloseFile 0
  DecodeShapes 0,m
  widths{"decodeshapes"}
EndIf
KillFile "RAM:bo999.shp"
End
