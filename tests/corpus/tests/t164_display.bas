; t164_display - displaylib in Amiga mode: coplists are built and changed
; in memory only (CreateDisplay, the only command that touches the
; hardware, is Blitz mode only and not used).  InitCopList in both forms
; (resolutions, fetch modes, AGA colours, EHB, HAM, HAM8, dual
; playfield, smooth scrolling, sprite resolution, genlock, plain and per
; line custom areas), DisplayBitMap (4 forms, HAM8, smooth), DisplaySprite
; (a sprite object poked here: visible, clipped at the top, off the
; bottom, custom sprite channels), DisplayPalette (3 forms, ECS/AGA),
; DisplayControls, DisplayAdjust, CustomColors (2 forms), CustomString,
; CustomSprites, CustomChunky (2 forms), CustomChunkySize,
; DisplayDblScan (copy, double, clear), DisplayRainbow, DisplayRGB,
; DisplayUser, DisplayScroll, DisplayGenlock.  Printed per coplist: the
; header fields, section offsets and a checksum of the list in which the
; data of pointer moves ($E0-$FE, $120-$17E) does not count; bitplane
; pointers as bitmap.plane+offset, sprite moves separately.
DEFTYPE .l
BitMap 0,320,32,4
BitMap 1,320,32,4
BitMap 2,64,8,8
InitPalette 0,256
For i=0 To 255 : AGAPalRGB 0,i,(i*7) AND 255,(i*13+5) AND 255,255-i : Next
Dim sd.l(63) : For i=0 To 63 : sd(i)=i*$01010101 : Next
s=Addr Sprite(0)
Poke.l s,&sd(0) : Poke.w s+4,10 : Poke.w s+6,1 : Poke.w s+8,0
Poke.w s+10,44 : Poke.w s+12,2 : Poke.w s+14,1
Dim dx.q(20),dy.q(20)
For i=0 To 20 : dx(i)=i*0.5-3 : dy(i)=1.25-i*0.25 : Next

; quick InitCopList: one per type
Restore types
For k=1 To 14
  Read ty
  InitCopList 7,ty : cn=7 : Print "type ",Hex$(ty)," " : Gosub info
Next
DisplayGenlock 4 : InitCopList 7,3 : DisplayGenlock 0
cn=7 : Print "genlock " : Gosub info

; A: AGA, plain custom area
InitCopList 0,44,32,$10004,8,32,80
cn=0 : Gosub info
; the blank sprite's address: in the SPR0POS/CTL data of a fresh list (BUG(orig) (#413))
p=Peek.l(Addr CopList(0)+12) : mh=Peek.w(p+2) AND $FFFF : ml=Peek.w(p+6) AND $FFFF
ms=(mh LSL 16) | ml
DisplayBitMap 0,0 : Gosub info : Gosub planes
DisplayBitMap 0,0,17.5,3 : Gosub info : Gosub planes
DisplayBitMap 0,0,1 : Gosub info : Gosub planes
DisplayBitMap 0,0,3,2,1,5.75,4 : Gosub info : Gosub planes
DisplayPalette 0,0 : Gosub info
DisplayPalette 0,0,4 : Gosub info
DisplayPalette 0,0,4,1 : Gosub info
DisplayControls 0,$10,$20,$30 : Gosub info
DisplayAdjust 0,1,2,3,4,5 : Gosub info
CustomColors 0,0,5,0,8,4 : Gosub info
CustomColors 0,12,9,0,0,2,1 : Gosub info
CustomString 0,20,12,Mkl$($01800FFF)+Mkl$($01820F00) : Gosub info
; BUG(orig) (#324): CustomSprites has !libs{#spriteslib,$1181} (Sprite object
; from parameter d1 = CCOffset): the compiler shifts d1 by the object
; shift (4) before the call, so CCOffset 1 means 64 bytes (and must be a
; valid Sprite number for the maximum check)
CustomSprites 0,1,14,2 : Gosub info

; B: AGA, per line custom area (9 longs a line)
InitCopList 1,44,20,$10003,8,16,-8
cn=1 : Gosub info
DisplayBitMap 1,0,8.25,0 : Gosub info
c=Addr CopList(1) : NPrint "x ",Peek.l(c+62)," ",Peek.l(c+66)
DisplayDblScan 1,2 : Gosub info
DisplayDblScan 1,1,3 : Gosub info
DisplayDblScan 1,0,6 : Gosub info
DisplayDblScan 1,4,6 : Gosub info
DisplayRainbow 1,1,0,2 : Gosub info
DisplayRainbow 1,40,0 : Gosub info
DisplayRGB 1,2,3,15,8,1,4 : Gosub info
DisplayRGB 1,33,5,1,2,3 : Gosub info
DisplayUser 1,5,Mkl$($01800123)+Mkl$($01820456),6 : Gosub info
DisplayUser 1,6,Mkl$($01800FFF) : Gosub info
DisplayScroll 1,&dx(0),&dy(0),0 : Gosub info
DisplayScroll 1,&dy(0),&dx(0) : Gosub info

; C: chunky
InitCopList 2,44,40,$10001,0,0,200
cn=2 : Gosub info
CustomChunky 2,0,10,8,4,2 : Gosub info
c=Addr CopList(2) : Print "chunky ",Peek.l(c+96)-Peek.l(c+4)," ",Peek.w(c+100)
NPrint " ",Peek.w(c+102)," ",Peek.w(c+104)," ",Peek.w(c+106)," ",Peek.w(c+108)
CustomChunky 2,60,20,8,4,2,2 : Gosub info
Print "chunky2 ",Peek.l(c+96)-Peek.l(c+4)," ",Peek.w(c+104)
NPrint " ",Peek.l(c+112)-Peek.l(c+4)," ",Peek.w(c+116)," ",Peek.w(c+120)," ",Peek.w(c+124)
NPrint "chunkysize ",CustomChunkySize(8,4)," ",CustomChunkySize(100,3)

; D: ECS, per line custom area (7 longs a line)
InitCopList 3,44,20,$3,8,8,-6
cn=3 : Gosub info
DisplayPalette 3,0 : Gosub info
DisplayRainbow 3,1,0 : Gosub info
DisplayRGB 3,2,4,15,0,7 : Gosub info

; F: HAM8 (8 planes, AGA colours)
InitCopList 5,$10088
cn=5 : Gosub info
DisplayBitMap 5,2 : Gosub info : Gosub planes
DisplayBitMap 5,2,3,1 : Gosub info : Gosub planes
DisplayPalette 5,0 : Gosub info

; G: smooth scrolling (DisplayBitMap without x,y uses 0,0)
InitCopList 6,$13
cn=6 : Gosub info
DisplayBitMap 6,0 : Gosub info : Gosub planes
DisplayBitMap 6,0,1 : Gosub info : Gosub planes
DisplaySprite 0,0,50,20,0 : Gosub sprites
DisplaySprite 0,0,-5.5,-3,1 : Gosub sprites
DisplaySprite 0,0,10,40,2 : Gosub sprites
; attached: two channels (an odd count would make the DBF run on)
Poke.w s+6,2 : Poke.w s+8,-1
DisplaySprite 0,0,100.25,7,3 : Gosub sprites
Poke.w s+6,1 : Poke.w s+8,0
DisplaySprite 0,0,12,2,9 : Gosub sprites
NPrint "done"
End

types:
Data.l $3,$13,$24,$46,$80,$104,$203,$1004,$2004,$3004,$406,$C06,$10006,$10288

; header, section offsets, checksum
info:
  c=Addr CopList(cn) : b=Peek.l(c+4)
  Print "cl",cn," ",Peek.l(c)
  For i=36 To 60 Step 2 : Print " ",Peek.w(i+c) : Next
  For i=70 To 90 Step 2 : Print " ",Hex$(Peek.w(i+c) AND $FFFF) : Next
  Print " ",Peek.w(c+126)
  For i=8 To 32 Step 4 : Print " ",Peek.l(i+c)-b : Next
  cs=0
  For i=0 To Peek.l(c)/4-1
    v=Peek.l(b+i*4) : r=(v LSR 16) AND $FFFF
    If (r AND 1)=0 AND ((r>=$E0 AND r<=$FE) OR (r>=$120 AND r<=$17E)) Then v=r
    cs=(cs LSL 1)+cs+v
  Next
  NPrint " cs ",Hex$(cs)
Return

; bitplane pointers: bitmap.plane+offset
planes:
  c=Addr CopList(cn) : p=Peek.l(c+20) : Print "planes"
  For i=0 To Peek.w(c+38)-1
    v=((Peek.w(p+i*8+2) AND $FFFF) LSL 16) | (Peek.w(p+i*8+6) AND $FFFF)
    f$="?"
    For bn=0 To 2
      bm=Addr BitMap(bn)
      For j=0 To Peek.b(bm+5)-1
        o=v-Peek.l(bm+8+j*4)
        If o>=0 AND o<(Peek.w(bm) AND $FFFF)*Peek.w(bm+2) Then f$=Str$(bn)+"."+Str$(j)+"+"+Str$(o)
      Next
    Next
    Print " ",Right$(Hex$(Peek.w(p+i*8)),3),"=",f$
  Next
  NPrint " mods ",Peek.w(p-6)," ",Peek.w(p-2)," con1 ",Hex$(Peek.w(p-10) AND $FFFF)
Return

; sprite moves of coplist 0 (16 + 2 custom channels): pos, ctl, pointer
; relative to the sprite data (M = the blank sprite, ? = other address)
sprites:
  c=Addr CopList(0)
  For k=0 To 9
    If k<8 Then p=Peek.l(c+12)+k*16 Else p=Peek.l(c+92)+(k-8)*16
    v=((Peek.w(p+10) AND $FFFF) LSL 16) | (Peek.w(p+14) AND $FFFF)
    ; pointer: into the sprite data (offset), the blank sprite (M), 0, or
    ; anything else (memory dependent: ?)
    f$="?" : o=v-&sd(0)
    If o>=0 AND o<256 Then f$=Str$(o)
    If v=ms Then f$="M"
    If v=0 Then f$="0"
    po=Peek.w(p+2) AND $FFFF : ct=Peek.w(p+6) AND $FFFF
    If po=mh AND ct=ml Then g$="MM" Else g$=Hex$(po)+"/"+Hex$(ct)
    Print " ",g$,"/",f$
  Next
  NPrint ""
Return
