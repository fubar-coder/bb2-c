; rifxlib-depth-word-at-4 (#670) - ClearBitmap (also FadeInBitmap and
; ZoomyZoom) reads the depth as the word at offset 4 of the BitMap,
; Flags * 256 + Depth.  A bitmap of one plane with the Flags byte 1
; (BMF_CLEAR) makes ClearBitmap work on 257 planes, through plane
; pointers far behind the BitMap and over its 16-entry pointer arrays.
; Made safe: the bitmap object is redirected to a block of the program's
; own (item 0 of the bitmaps: the pointers in the globals that hold the
; address of bitmap 0 are replaced for the call), with Flags 1, Depth 1;
; plane 0 points to buffer A, the 256 pointers behind it to buffer B
; (640 bytes each, BytesPerRow 40 x Rows 16, filled with $AA between
; margins of $AA), so no plane write leaves the two buffers.  The plane
; pointer arrays of the library are overrun by 1 KB of what lies behind
; them (the constants of the program, then foreign memory): the 3.5 KB
; around the library's data are saved before the call and put back
; right after it, before any constant is used.  The program
; goes on and reports which buffer was cleared.
; Correct: the depth is the byte at 5, one plane is cleared: A is
; cleared, B stays.
; @rwe rifxlib
DEFTYPE .l
Dim h(15)
BitMap 0,320,16,1
v=0 : a=&v
ob=Addr BitMap(0)+1                 ; +1: not found by the scan below
bs=8+257*4
buf=AllocMem_(bs,$10001)
sa=AllocMem_(640+128,$10001)
sb=AllocMem_(640+128,$10001)
sv=AllocMem_(3584,$10001)
If buf=0 OR sa=0 OR sb=0 OR sv=0 Then NPrint "no memory" : End
gt=GetZoomTables-1024               ; the library's own data and what follows it
For i=0 To 191
  Poke.l sa+i*4,$AAAAAAAA : Poke.l sb+i*4,$AAAAAAAA
Next
Poke.l buf+8,sa+64
For i=1 To 256 : Poke.l buf+8+i*4,sb+64 : Next
nh=0
For i=-256 To 256
  p=a+i*4
  If Peek.l(p)=ob-1 AND nh<16 Then h(nh)=p : nh=nh+1 : Poke.l p,buf
Next
If Addr BitMap(0)<>buf
  NPrint "object not redirected"
  For j=0 To nh-1 : Poke.l h(j),ob-1 : Next
  End
EndIf
Poke.w buf,40 : Poke.w buf+2,16 : Poke.b buf+4,1 : Poke.b buf+5,1 : Poke.w buf+6,0
NPrint "before"
CopyMem_ gt,sv,3584
ClearBitmap 0,1,1,0
ca=0 : cb=0
For i=0 To 191
  If Peek.l(sa+i*4)<>$AAAAAAAA Then ca=ca+1
  If Peek.l(sb+i*4)<>$AAAAAAAA Then cb=cb+1
Next
CopyMem_ sv,gt,3584
For j=0 To nh-1 : Poke.l h(j),ob-1 : Next
FreeMem_ buf,bs
FreeMem_ sa,640+128
FreeMem_ sb,640+128
FreeMem_ sv,3584
If ca>0 Then NPrint "A cleared" Else NPrint "A untouched"
If cb=0
  NPrint "B untouched"
Else
  NPrint "B cleared: ",cb," longs"
EndIf
End
