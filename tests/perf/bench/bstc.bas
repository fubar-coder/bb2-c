; bstc - perf benchmark: the S404 decruncher of the RWE userlib ristcdecrunchlib
; (optidec, shared with ristonecrackerlib): STCDecrunch to another address
; on real data (ancient's test_C1.pack410, 152089 bytes of text packed by
; StoneCracker 4.10) and on a file of one literal byte (the cost of a call)
; @rwe ristcdecrunchlib
; @file data/test_C1.pack410|@ANCIENT@/testing/test_files/test_C1.pack410
DEFTYPE .l
Dim t(4)
c=0
f=AllocMem_(64892,1)
d=AllocMem_(152089,1)
s=AllocMem_(64,$10001)
If f=0 OR d=0 OR s=0 Then NPrint "no memory" : End
If ReadFile(0,"CORPUS:data/test_C1.pack410")
  ReadMem 0,f,64892
  CloseFile 0
EndIf
; S404, offset 0, unpacked 1, packed 4; trailer: 4 far bits, the bit
; buffer $2080 with 9 bits: "0" + $41 (a literal "A")
Poke.l s,$53343034 : Poke.l s+8,1 : Poke.l s+12,4
Poke.w s+16,4 : Poke.w s+18,$2080 : Poke.w s+20,9
VWait 5
BLITZ
Poke.b $bfde00,0 : Poke.b $bfdf00,0
Poke.b $bfd400,99 : Poke.b $bfd500,0
Poke.b $bfd600,$ff : Poke.b $bfd700,$ff
Poke.b $bfdf00,$51 : Poke.b $bfde00,$11
VWait 2
Gosub tod : t0=tt
For i=1 To 20000
Next
Gosub tod : t(0)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 150000
Next
Gosub tod : t(1)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 1
c=STCDecrunch(f,d)
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=STCDecrunch(s,s+32)
Next
Gosub tod : t(3)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "c1pack410 ",t(2)," 1 1"
NPrint "onebyte ",t(3)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return
