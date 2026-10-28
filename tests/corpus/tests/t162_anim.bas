; t162_anim - animlib with an ANIM built here (16x4, 2 planes, ByteRun1
; BODY with literal/repeat/$80 runs, 3 op 5 DLTAs with skip/uniq/same
; column ops, one plane skipped per delta) written to RAM:, once with
; MOVE deltas, once with EOR deltas (ANHD bits = 2).  LoadAnim (both
; forms; the palette form reads the CMAP through palettelib), Frames,
; InitAnim into the used bitmap and with a second bitmap, NextFrame
; over the end (wraps to the second delta), loading over a loaded anim.
; Expected planes (host model of the original, plane 0 / plane 1):
;   init 0050105120523053 0101117121213173
;   MOVE 000FAA0FBB0F300F 0101117121213173, 0033AA0FBB0F300F F001F07121113122,
;        0033AA0FBB0F300F 0101027103110477, then the last two alternate
;   EOR  005FBA5E9B5D305C 0101117121213173, 006CBA5E9B5D305C F101E17121303151,
;        006CBA5E9B5D305C F001E37122303526, 005FBA5E9B5D305C 0001137122213504,
;        005FBA5E9B5D305C 0101117121213173
DEFTYPE .l
Dim m.l(160) : b=&m(0) : n=0
Restore animdata
For k=1 To 17
  Read h$
  For i=1 To Len(h$) Step 2
    Poke.b b+n,Val("$"+Mid$(h$,i,2)) : n=n+1
  Next
Next
NPrint "bytes ",n
If OpenFile(0,"RAM:t162a.anim") Then WriteMem 0,b,n : CloseFile 0
Poke.b b+153,2
Poke.b b+295,2
Poke.b b+441,2
If OpenFile(0,"RAM:t162b.anim") Then WriteMem 0,b,n : CloseFile 0
BitMap 0,16,4,2
BitMap 1,16,4,2
LoadAnim 0,"RAM:t162a.anim"
an=0 : a=Addr Anim(0) : Gosub info
Use BitMap 0
InitAnim 0 : bn=0 : Gosub dump
For k=1 To 5 : NextFrame 0 : Gosub dump : Next
InitAnim 0,1 : bn=1 : Gosub dump : bn=0 : Gosub dump
InitPalette 0,4
LoadAnim 1,"RAM:t162b.anim",0
an=1 : a=Addr Anim(1) : Gosub info
p=Peek.l(Addr Palette(0))
NPrint "palette ",p<>0," ",Peek.b(p+4+12) AND 255," ",Peek.b(p+4+24+4) AND 255," ",Peek.b(p+4+36+8) AND 255
InitAnim 1 : Gosub dump
For k=1 To 5 : NextFrame 1 : Gosub dump : Next
; load over a loaded anim (frees the old one first)
LoadAnim 0,"RAM:t162b.anim"
an=0 : a=Addr Anim(0) : Gosub info
InitAnim 0 : NextFrame 0 : Gosub dump
KillFile "RAM:t162a.anim" : KillFile "RAM:t162b.anim"
NPrint "done"
End

info:
  Print "anim ",Peek.w(a)," ",Peek.w(a+2)," ",Peek.w(a+4)," ",Peek.w(a+6)
  Print " frames ",Frames(an)," ",Peek.w(a+8)," body ",Peek.l(a+14)
  NPrint " eor ",Peek.w(a+30)," deltas ",Peek.l(a+18)<>0," cur ",Peek.l(a+26)
Return

dump:
  bm=Addr BitMap(bn) : Print "bm",bn
  For pl=0 To 1
    pa=Peek.l(bm+8+pl*4) : Print " "
    For i=0 To 7 : Print Right$(Hex$(Peek.b(pa+i) AND 255),2) : Next
  Next
  NPrint ""
Return

animdata:
Data$ "464F524D00000216414E494D464F524D0000005A494C424D424D484400000014"
Data$ "00100004000000000200010000000A0B014000C8434D41500000000C000000FF"
Data$ "000000FF000000FF424F44590000001E010050FF018000100051808001117101"
Data$ "2052FF2180003000538080013173464F524D00000086494C424D414E48440000"
Data$ "0028050000100004000000000000000000000001000000000000000000000000"
Data$ "00000000000000000000444C54410000004A0000004000000000000000000000"
Data$ "0000000000000000000000000000000000000000000000000000000000000000"
Data$ "000000000000000000000000000000000000020182AABB0100040F00464F524D"
Data$ "0000008A494C424D414E48440000002805000010000400000000000000000000"
Data$ "000100000000000000000000000000000000000000000000444C54410000004E"
Data$ "0000004000000044000000000000000000000000000000000000000000000000"
Data$ "0000000000000000000000000000000000000000000000000000000000000000"
Data$ "00018133010002F0020282112200464F524D00000088494C424D414E48440000"
Data$ "0028050000100004000000000000000000000001000000000000000000000000"
Data$ "00000000000000000000444C54410000004C0000000000000040000000000000"
Data$ "0000000000000000000000000000000000000000000000000000000000000000"
Data$ "000000000000000000000000000000000000018401020304020300017700"
