; ripacklib-loadiff-bank-51 (#770) - LoadIFF loads the file into the first
; free bank of RIAmosFuncLib from bank 51 on (NextBank(51)), but
; RIAmosFuncLib has 50 banks unless BankLimit raised the number: there is
; no bank 51, LoadIFF returns 0 and loads nothing.  The file is a 16x1
; ILBM with one plane, its row $FF,$00.
; Correct: the picture is loaded with the default number of banks: -1,
; pixel 0 has colour 1, pixel 8 colour 0.
; @fixed #770
; @rwe ripacklib riamosfunclib
; @pass file=p.iff|@TESTDATA@/ripacklib-loadiff-bank-51.iff
DEFTYPE .l
BitMap 0,320,16,1
r=LoadIFF("CORPUS:p.iff",0)
NPrint r," ",Point(0,0)," ",Point(8,0)
End
