; shapeslib-word-arithmetic (#263) - LoadShape keeps the sizes of a shape
; as words: +26 (whole shape) is the low word of bytes per plane x depth.
; Here: an ILBM of 640x200 with 5 planes (16000 bytes per plane, 80000
; in all; uncompressed, built in memory and written to CORPUS:) is loaded;
; +26 is 80000 mod 65536 = 14464, and SaveShapes writes only that many
; bytes of the data behind the 32 byte block (file 14496 bytes).
; Correct: the whole shape is saved: 32 + 80000 = 80032 bytes.
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

!TestCase{plane_80000}
  n=80000
  m=AllocMem(n+256,65536) : p=0
  pl{$464F524D} : pl{0} : pl{$494C424D}
  pl{$424D4844} : pl{20}
  pw{640} : pw{200} : pw{0} : pw{0} : pb{5} : pb{0} : pb{0} : pb{0}
  pw{0} : pb{10} : pb{11} : pw{640} : pw{200}
  pl{$424F4459} : pl{n}
  For i=0 To 79 : Poke.b m+p+i,i+1 : Next
  p=p+n
  Poke.l m+4,p-8
  ok=WriteFile(0,"CORPUS:bo263.iff")
  !Require{ok,"file_written"}
  WriteMem 0,m,p
  CloseFile 0
  FreeMem m,n+256
  LoadShape 0,"CORPUS:bo263.iff"
  KillFile "CORPUS:bo263.iff"
  a=Addr Shape(0)
  !CheckEq{Peek.w(a),640,"shape_width"}
  !CheckEq{Peek.w(a+2),200,"shape_height"}
  !CheckEq{Peek.w(a+4),5,"shape_depth"}
  !CheckEq{Peek.w(a+22) AND $FFFF,16000,"per_plane"}
  SaveShapes 0,0,"CORPUS:bo263.shp"
  ok=ReadFile(0,"CORPUS:bo263.shp")
  !Require{ok,"shapes_file_read"}
  !CheckEq{Lof(0),80032,"file_size"}
  CloseFile 0
  KillFile "CORPUS:bo263.shp"
!EndTestCase
