; t110_audio - audiolib: InitSound, SoundData, PeekSound, Sound, LoopSound,
; Volume, Filter, SetPeriod, DecodeSound (8SVX built in memory), LoadSound
; (8SVX from the build), the Sound objects' fields.  Audio output is not
; checked (write-only hardware); Sound/LoopSound only wait for the beam.
; Sounds are played with zero data or volume 0 (nothing audible).
; DiskPlay is not used: it waits for audio interrupts.
; PeekSound is used in plain assignments here; an object function inside
; a string expression is checked by the bug-orig test
; compiler-libfunc-a3-string (#46).
DEFTYPE .l
Function.l cks{a.l,n.l}
  c.l=0
  For i.l=0 To n-1
    c=(c*3+(Peek.b(a+i) AND 255)) AND $FFFFFF
  Next
  Function Return c
End Function
Statement info{n.l}
  s.l=Addr Sound(n)
  d.l=Peek.l(s)
  r.l=Peek.l(s+8)
  Print "snd ",n," data ",d<>0," per ",Peek.w(s+4)," len ",Peek.w(s+6)," rep ",r-d," replen ",Peek.w(s+12)
  NPrint " mark ",Hex$(Peek.w(s+14))
End Statement
InitSound 0,32
info{0}
; play while the data is still zero
Sound 0,1
Sound 0,2,0
Sound 0,12,0,0
LoopSound 0,4
LoopSound 0,9,0,0
Volume 15,0,0,0,0
Volume 3,0,0
NPrint "played"
For i=0 To 31 : SoundData 0,i,i*8-128 : Next
For i=0 To 31 Step 4
  p=PeekSound(0,i)
  Print p," "
Next
NPrint ""
NPrint "cks ",Hex$(cks{Peek.l(Addr Sound(0)),32})
InitSound 1,64,300
info{1}
InitSound 2,100,500,40
info{2}
p=PeekSound(2,0) : q=PeekSound(2,99)
NPrint "peek new ",p," ",q
SetPeriod 0,10
info{0}
; filter (the LED bit of CIA-A PRA)
Filter 1 : f1=Peek.b($BFE001) AND 2
Filter 0 : f0=Peek.b($BFE001) AND 2
NPrint "filter ",f1," ",f0
; DecodeSound of an 8SVX FORM in memory
Dim m.l(40)
a=&m(0)
Poke.l a,$464F524D : Poke.l a+4,56 : Poke.l a+8,$38535658
Poke.l a+12,$56484452 : Poke.l a+16,20
Poke.l a+20,8 : Poke.l a+24,8 : Poke.l a+28,0 : Poke.w a+32,8000 : Poke.w a+34,$100 : Poke.l a+36,$10000
Poke.l a+40,$424F4459 : Poke.l a+44,16
For i=0 To 15 : Poke.b a+48+i,i*7 : Next
DecodeSound 3,a
info{3}
s=Addr Sound(3)
NPrint "decoded same ",Peek.l(s)=a+48," cks ",Hex$(cks{Peek.l(s),16})
; a chunk before VHDR is skipped
Poke.l a+12,$4E414D45 : Poke.l a+16,0
Poke.l a+20,$56484452 : Poke.l a+24,20
Poke.l a+28,2 : Poke.l a+32,6 : Poke.l a+36,0 : Poke.w a+40,10000 : Poke.w a+42,$100 : Poke.l a+44,$10000
Poke.l a+48,$424F4459 : Poke.l a+52,8 : Poke.l a+4,64
For i=0 To 7 : Poke.b a+56+i,100-i : Next
DecodeSound 4,a
info{4}
NPrint "decoded 4 cks ",Hex$(cks{Peek.l(Addr Sound(4)),8})
; LoadSound of two generated files (IFF 8SVX through iffiolib)
LoadSound 5,"CORPUS:data/t110_bounce.8svx"
info{5}
NPrint "loaded cks ",Hex$(cks{Peek.l(Addr Sound(5)),Peek.w(Addr Sound(5)+6)*2})
LoadSound 5,"CORPUS:data/t110_hitwall.8svx"
info{5}
NPrint "loaded cks ",Hex$(cks{Peek.l(Addr Sound(5)),Peek.w(Addr Sound(5)+6)*2})
Sound 5,15,0,0,0,0
; re-init frees the old data
InitSound 0,16,214,8
info{0}
p=PeekSound(0,0)
NPrint "peek after reinit ",p
NPrint "end"
End
