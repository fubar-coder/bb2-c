; t60_crunch - crunchlib: Implode/Deplode round trips (empty, 1 byte, too
; short, repetitive, random, small alphabet, large), PPDecrunch and
; CrMDecrunch (CrM! and CrM2, in place and to a destination; the packed
; data were made by a test encoder for the same text that is built below)
Dim m.l(12000)
b.l=&m(0)
Function.l cksum{a.l,n.l}
  c.l=0
  For i.l=0 To n-1
    c=(c*3+(Peek.b(a+i) AND 255)) AND $FFFFFF
  Next
  Function Return c
End Function
Statement fill{a.l,n.l,k.l,s.l}
  ; k=0: repetitive, 1: random, 2: alphabet of 4, 3: mixed
  For i.l=0 To n-1
    s=(s*75+74) MOD 65537
    v.l=0
    If k=0
      v=i MOD 23 : If v>11 Then v=65
    EndIf
    If k=1 Then v=s AND 255
    If k=2 Then v=s AND 3
    If k=3
      v=(i*7) AND 255
      If (i MOD 97)<50 Then v=s AND 3
    EndIf
    Poke.b a+i,v
  Next
End Statement
Statement imptest{a.l,n.l,k.l}
  fill{a,n,k,n+1}
  For i.l=n To n+300 : Poke.b a+i,0 : Next
  c1.l=cksum{a,n}
  r.l=Implode(a,n)
  If r>0
    NPrint "implode ",n," kind ",k," -> ",r," hdr ",Hex$(Peek.l(a))," ",Peek.l(a+4)," ",Peek.l(a+8)," ck ",Hex$(cksum{a,r})
    d.l=Deplode(a)
    c2.l=cksum{a,n}
    NPrint "  deplode ",d," same ",c1=c2
  Else
    NPrint "implode ",n," kind ",k," -> ",r," ck before ",Hex$(c1)," after ",Hex$(cksum{a,n})
  EndIf
End Statement
NPrint "deplode not imp ",Deplode(b)
imptest{b,0,0}
imptest{b,1,0}
imptest{b,63,0}
imptest{b,64,0}
imptest{b,1000,0}
imptest{b,2000,1}
imptest{b,3000,2}
imptest{b,20000,3}
imptest{b,301,2}
; text for the decrunchers
t$=""
For i=1 To 10 : t$=t$+"Blitz Basic 2 crunchlib test "+Chr$(64+i)+". " : Next
tl.l=Len(t$)
Poke.s b+40000,t$
ct.l=cksum{b+40000,tl}
NPrint "text ",tl," ",Hex$(ct)
; packed data: PP20 at b, CrM! at b+100, CrM2 at b+200
Restore pdat
For i=0 To 21 : Read v.l : Poke.l b+i*4,v : Next
Restore cdat
For i=0 To 24 : Read v.l : Poke.l b+100+i*4,v : Next
Restore zdat
For i=0 To 22 : Read v.l : Poke.l b+200+i*4,v : Next
For i=0 To 399 : Poke.b b+20000+i,0 : Next
PPDecrunch b,b+88,b+20000
NPrint "pp ",cksum{b+20000,tl}=ct," ",Peek.b(b+20000+tl)
For i=0 To 399 : Poke.b b+20000+i,0 : Next
CrMDecrunch b+100,b+20000
NPrint "crm ",cksum{b+20000,tl}=ct," ",Peek.b(b+20000+tl)
For i=0 To 399 : Poke.b b+20000+i,0 : Next
CrMDecrunch b+200,b+20000
NPrint "crm2 ",cksum{b+20000,tl}=ct," ",Peek.b(b+20000+tl)
; in place: the data are first copied MinSecDist bytes below the buffer
For i=0 To 24 : Poke.l b+30000+i*4,Peek.l(b+100+i*4) : Next
CrMDecrunch b+30000
NPrint "crm in place ",cksum{b+30000,tl}=ct
For i=0 To 22 : Poke.l b+30000+i*4,Peek.l(b+200+i*4) : Next
CrMDecrunch b+30000
NPrint "crm2 in place ",cksum{b+30000,tl}=ct
; not packed: nothing happens
Poke.l b+30000,$12345678
CrMDecrunch b+30000
PPDecrunch b+30000,b+30100,b+20000
NPrint "not packed ",Hex$(Peek.l(b+30000))," ",cksum{b+20000,tl}=ct
End
pdat:
Data.l $50503230,$090A0B0C,$DFFF8E08,$1FFFC740,$1BFFF8F0,$85FFFC64
Data.l $42FFFE3A,$217FFF1B,$10BFFF8F,$885FFFC6,$242FFFE3,$92749402
Data.l $900B9781,$10A1B3A5,$B1811301,$3193AB9D,$B1858DA5,$91810BA9
Data.l $B38B8114,$9D012FFF,$FC000000,$00014019
cdat:
Data.l $43724D21,$00640000,$01400000,$00540001,$B9060003,$F0D00004
Data.l $721C0008,$E4480011,$C8B00023,$91A00047,$23C0008E,$488018CC
Data.l $6D0D8D2C,$440E8CAE,$6E840925,$C4084D8D,$2E8F4408,$4C2E6D2C
Data.l $6406440C,$6E4EADCC,$6D0D8D2C,$440E8CAE,$6E840945,$C41FFB00
Data.l $00090000
zdat:
Data.l $43724D32,$005C0000,$01400000,$004C7829,$F02F0CF8,$17C2BE0D
Data.l $F0EF857E,$0DC610DB,$BF843CDA,$A172AF7C,$FADC6D3C,$E5E7D3B8
Data.l $C003348D,$EAEB72B7,$5A2CB62B,$0D2A9348,$995CED94,$7A351689
Data.l $43A0CB83,$574B4D8E,$85200E18,$A4418000,$00020000
