; bmod - perf benchmark: the PlayModule interrupt per frame, PlayMed per call
DEFTYPE .l
Dim t(4)
c=0
LoadModule 0,"CORPUS:data/t111_song.mod"
LoadMedModule 0,"CORPUS:data/t112_song.med"
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
PlayModule 0
Gosub tod : t0=tt
For i=1 To 150000
Next
Gosub tod : t(1)=(tt-t0)&65535
StopModule
StartMedModule 0
Gosub tod : t0=tt
For i=1 To 2000
PlayMed
Next
Gosub tod : t(2)=(tt-t0)&65535
StopMed
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "vblmod ",t(1)," 150000 frame"
NPrint "playmed ",t(2)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return
