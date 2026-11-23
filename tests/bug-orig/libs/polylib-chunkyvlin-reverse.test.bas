; polylib-chunkyvlin-reverse (#417) - ChunkyVlin x,y1,y2,colour (without
; CLIP) counts the words as y2-y1 with DBRA and does not swap y1/y2 (only
; the CLIP form does): for y2 < y1 the counter is negative and about 65536
; words are written, one per chunky line.  Such a range is a programming
; error (the plain form is the fast form, without tests), and the
; original has no error check for it, not even with runtime error
; checking.
; Here with runtime error checking: the chunky area of CopList 0
; (CustomChunky, never displayed) is pointed at a zeroed block of 140000
; bytes with 2 bytes per chunky line (CopList +96 area, +108 bytes per
; line), so that 65536 lines stay inside it; ChunkyVlin 0,40,50 (no
; error), then ChunkyVlin 0,50,40.  Counted: words written.
; Correct: the error check refuses the second line: "ChunkyVlin y1 MUST
; NOT be greater than y2".  The original writes about 65536 words.
; The error is announced with !ExpectError; the original raises none (see
; the configuration).
DEFTYPE .l
Function.l nset{m.l}
  n.l=0
  For i.l=0 To 139998 Step 2
    If Peek.w(m+i)<>0 Then n=n+1
  Next
  Function Return n
End Function

!TestCase{reverse_range}
  cs=CustomChunkySize(64,16)
  InitCopList 0,44,64,1,0,2,cs
  CustomChunky 0,0,8,64,16,1
  c=Addr CopList(0)
  ch=Peek.l(c+96) : cc=Peek.w(c+108)
  m=AllocMem(140000,$10000)
  Poke.l c+96,m : Poke.w c+108,2
  ChunkyVlin 0,40,50,$FFF
  !CheckEq{nset{m},11,"forty_to_fifty"}
  !ExpectError{"ChunkyVlin y1 MUST NOT be greater than y2","refused"}
  ChunkyVlin 0,50,40,$FFF
  !Check{0,"after"}
!EndTestCase
