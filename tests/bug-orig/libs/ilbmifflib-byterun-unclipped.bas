; ilbmifflib-byterun-unclipped (#191) - DecodeILBM writes a ByteRun1 run
; completely even when it goes past the end of the row; only the row
; counter is lowered.
; Here: an ILBM in memory, 16x2 with 1 plane, ByteRun1: row 0 a copy of
; 2 bytes (11 22), row 1 a repeat of 4 bytes AA (2 more than the row
; has), decoded into a 16x4 bitmap.  The original writes the rest of the
; run into bitmap row 2, below the picture.
; Correct: the run is cut at the end of the row; rows 2 and 3 stay 0.
; @configs clib
DEFTYPE .l
Statement pb{v.l}
  SHARED m,p
  Poke.b m+p,v : p=p+1
End Statement
Statement pw{v.l}
  pb{(v LSR 8) AND 255} : pb{v AND 255}
End Statement
Statement pl{v.l}
  pw{(v LSR 16) AND $FFFF} : pw{v AND $FFFF}
End Statement
m=AllocMem(256,65536) : p=0
pl{$464F524D} : pl{0} : pl{$494C424D}
pl{$424D4844} : pl{20}
pw{16} : pw{2} : pw{0} : pw{0} : pb{1} : pb{0} : pb{1} : pb{0}
pw{0} : pb{10} : pb{11} : pw{320} : pw{200}
pl{$424F4459} : pl{5}
pb{1} : pb{$11} : pb{$22}
pb{$FD} : pb{$AA}
pb{0}
Poke.l m+4,p-8
BitMap 0,16,4,1
DecodeILBM 0,m
b=Peek.l(Addr BitMap(0)+8)
For r=0 To 3
  NPrint "row ",r," ",Right$(Hex$(Peek.w(b+r*2) AND $FFFF),4)
Next
FreeMem m,256
End
