; polylib-chunkyvlin-clip-x (#987) - ChunkyVlin x,y1,y2,colour,CLIP rejects
; a negative x and clips y1/y2 against the chunky height, but never
; compares x with the chunky width (ChunkyPlot with CLIP does).  The word
; of a pixel is at (x AND 31)*4 + (x/32)*132 + y*bytes per line from the
; start of the chunky area, so an x beyond the width writes into other
; rows or behind the area.
; Here the chunky area of CopList 0 (CustomChunky 64 pixels wide, 16
; high, never displayed) is pointed at a zeroed block of 8000 bytes with
; 2 bytes per chunky line (CopList +96 area, +108 bytes per line), so
; that the writes stay inside the block; ChunkyVlin 100,0,15,$FFF,CLIP
; (x beyond the width).  Counted: words written.  Control: the same with
; x = 5 (inside) writes 16 words.
; Correct: nothing is written for x = 100 (clipped away).
; @pass timeout=60
DEFTYPE .l
cs=CustomChunkySize(64,16)
InitCopList 0,44,64,1,0,2,cs
CustomChunky 0,0,8,64,16,1
c=Addr CopList(0)
ch=Peek.l(c+96) : cc=Peek.w(c+108)
m=AllocMem(8000,$10000)
Poke.l c+96,m : Poke.w c+108,2
ChunkyVlin 100,0,15,$FFF,CLIP
n=0
For i=0 To 7998 Step 2
  If Peek.w(m+i)<>0 Then n=n+1
Next
ChunkyVlin 5,0,15,$FFF,CLIP
k=-n
For i=0 To 7998 Step 2
  If Peek.w(m+i)<>0 Then k=k+1
Next
Poke.l c+96,ch : Poke.w c+108,cc
NPrint "x=100 (clipped): ",n," words"
NPrint "x=5: ",k," words"
FreeMem m,8000
End
