; shapeslib-loadshape-plane-step (#263) - LoadShape decodes the BODY row
; by row and plane by plane and goes from the end of a row of one plane to
; the same row of the next plane with ADD d7,a2, d7 = bytes per plane -
; bytes per row as a word (sign extended): for planes of 32K and more
; the step goes backwards and the planes from the second one on are
; written before the data block.
; Here: an ILBM of 640x420 with 2 planes (33600 bytes per plane, the step
; 33520), uncompressed, built in memory by the batch init and written to
; CORPUS:; the first byte of row 0 of plane 1 is $5A, of plane 0 $11.
; Setup: the case takes every chip block of 70000 bytes or more and gives
; back only a window of 300000 bytes inside the largest one, with 40000
; bytes (the guard area) before it, all of them set to $AA; the data block
; of the shape can only lie in the window, and what the original writes
; before it lies in the guard area.  Act: LoadShape.  Verify: the place of
; the data block in the window, the first byte of the two planes, the
; guard area in full as runs of equal bytes (value x length).
; Correct: plane 0 starts with $11, plane 1 with $5A, the guard area is
; 40000 bytes of $AA.  The original writes plane 1 (33600 bytes: $5A,
; then zeros) from 31936 bytes before the data block on: the guard area
; changes from there to its end and the first rows of plane 0 are
; overwritten with zeros (the deviations of libs=cd).
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

!TestBatchInit
  n=67200
  m=AllocMem(n+256,65537) : p=0
  !Require{m<>0,"memory"}
  pl{$464F524D} : pl{0} : pl{$494C424D}
  pl{$424D4844} : pl{20}
  pw{640} : pw{420} : pw{0} : pw{0} : pb{2} : pb{0} : pb{0} : pb{0}
  pw{0} : pb{10} : pb{11} : pw{640} : pw{420}
  pl{$424F4459} : pl{n}
  Poke.b m+p,$11 : Poke.b m+p+80,$5A
  p=p+n
  Poke.l m+4,p-8
  ok=WriteFile(0,"CORPUS:bo263s.iff")
  !Require{ok,"file_written"}
  WriteMem 0,m,p
  CloseFile 0
  FreeMem m,n+256
!EndTestBatchInit

!TestCase{plane_step_32k}
  ; setup: all large chip blocks, the window and the guard area before it
  Dim blk.l(40),sz.l(40)
  nb=0
  s=AvailMem_($20002)
  While s>=70000 AND nb<40
    b=AllocMem_(s,2)
    If b=0
      s=0
    Else
      blk(nb)=b : sz(nb)=s : nb=nb+1
      s=AvailMem_($20002)
    EndIf
  Wend
  !Require{nb>0,"chip_blocks"}
  c=blk(0)
  g=40000 : w=300000
  !Require{sz(0)>=g+w,"largest_block"}
  For i=0 To g+w-4 Step 4
    Poke.l c+i,$AAAAAAAA
  Next
  FreeMem_ c+g,w
  ; act
  LoadShape 0,"CORPUS:bo263s.iff"
  ; verify
  a=Addr Shape(0)
  d=Peek.l(a+14)
  !CheckEq{d-(c+g),8,"data_offset"}
  !CheckEq{Peek.b(d) AND 255,$11,"plane_0"}
  !CheckEq{Peek.b(d+(Peek.w(a+22) AND $FFFF)) AND 255,$5A,"plane_1"}
  v=Peek.b(c) AND 255 : k=0 : r$=""
  For i=0 To g-1
    x=Peek.b(c+i) AND 255
    If x=v
      k=k+1
    Else
      r$=r$+Hex$(v)+" x "+Str$(k)+";"
      v=x : k=1
    EndIf
  Next
  r$=r$+Hex$(v)+" x "+Str$(k)
  !CheckStrEq{r$,"000000AA x 40000","guard"}
  Free Shape 0
  FreeMem_ c,g
  FreeMem_ c+g+w,sz(0)-g-w
  For i=1 To nb-1 : FreeMem_ blk(i),sz(i) : Next
!EndTestCase

!TestBatchEnd
  KillFile "CORPUS:bo263s.iff"
!EndTestBatchEnd
