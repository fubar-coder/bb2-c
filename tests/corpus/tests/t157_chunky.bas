; t157_chunky - polylib chunky commands on a copper chunky area (displaylib
; InitCopList + CustomChunky, never displayed): ChunkyCLS, ChunkyPlot and
; ChunkyVlin (plain and clipping forms), ChunkyPoly (blitter and CPU form)
; and RenderToChunky + RenderPoly (colour table) in a short BLITZ ... AMIGA
; section (both Blitz mode only); checksums of the chunky area and some
; pixel words.
DEFTYPE .l
Statement setp{m.l,a$}
  While a$<>""
    i.l=Instr(a$,",")
    If i=0 Then i=Len(a$)+1
    Poke.w m,Val(Left$(a$,i-1)) : m=m+2
    a$=Mid$(a$,i+1)
  Wend
End Statement
Statement csum{c.l}
  ch.l=Peek.l(c+96) : n.l=Peek.w(c+104)*Peek.w(c+108) : s.l=0
  For k.l=0 To n-1 Step 2 : s=(s*5+Peek.w(ch+k))&$FFFFFF : Next
  Print " ",s
  For y.l=0 To 15 Step 5
    For x.l=0 To 63 Step 21
      Print " ",Hex$(Peek.w(ch+(x&31)*4+(x/32)*132+y*Peek.w(c+108)))
    Next
  Next
  NPrint ""
End Statement
cs=CustomChunkySize(64,16)
NPrint "size ",cs
InitCopList 0,44,64,1,0,2,cs
CustomChunky 0,0,8,64,16,1
c=Addr CopList(0)
NPrint "chunky ",Peek.w(c+102)," ",Peek.w(c+104)," ",Peek.w(c+106)," ",Peek.w(c+108)," ",Peek.l(c+96)-Peek.l(c+28)
ChunkyCLS 0 : Print "cls 0" : csum{c}
ChunkyCLS $123 : Print "cls" : csum{c}
ChunkyPlot 0,0,$F00 : ChunkyPlot 63,15,$0F0 : ChunkyPlot 31,5,1 : ChunkyPlot 32,5,2
ChunkyPlot -1,0,5,1 : ChunkyPlot 64,0,5,1 : ChunkyPlot 0,16,5,1 : ChunkyPlot 10,10,7,1
ChunkyPlot 0,-1,5,1 : ChunkyPlot 42,10,$ABC,0
Print "plot" : csum{c}
ChunkyVlin 5,2,10,$FFF : ChunkyVlin 40,3,3,$AAA : ChunkyVlin 63,0,15,$111
ChunkyVlin 50,-5,30,$555,1 : ChunkyVlin 20,12,3,$333,1 : ChunkyVlin -1,0,5,1,1
ChunkyVlin 30,16,20,$777,1 : ChunkyVlin 33,-9,-2,$888,1 : ChunkyVlin 34,14,40,$999,1
Print "vlin" : csum{c}
Dim p.w(100)
a=&p(0)
setp{a,"5,2,40,4,30,13,8,11"}
setp{a+20,"45,1,62,8,50,14"}
setp{a+40,"10,-5,30,20,0,10"}
setp{a+60,"50,5,90,9,55,30"}
ChunkyCLS 0
VWait
BLITZ
ChunkyPoly 4,a,$F0F : ChunkyPoly 3,a+20,$0FF,1
ChunkyPoly 3,a+40,$FF0 : ChunkyPoly 3,a+60,$00F,0
VWait 2
AMIGA
Print "poly" : csum{c}
ChunkyCLS 0
BLITZ
RenderToChunky 0
RenderPoly 4,a,1 : RenderPoly 3,a+20,7 : RenderPoly 3,a+40,14
VWait 2
AMIGA
Print "render" : csum{c}
End
