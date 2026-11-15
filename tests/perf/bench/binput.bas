; binput - perf benchmark: joystick, keyboard, mouse, Sound, Volume
DEFTYPE .l
Dim t(16)
c=0
r$=String$(" ",80)
InitSound 0,64
VWait 5
BLITZ
Poke.b $bfde00,0 : Poke.b $bfdf00,0
Poke.b $bfd400,99 : Poke.b $bfd500,0
Poke.b $bfd600,$ff : Poke.b $bfd700,$ff
Poke.b $bfdf00,$51 : Poke.b $bfde00,$11
BlitzKeys On
Mouse On
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
For i=1 To 2000
c=Joyx(1)
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Joyy(1)
Next
Gosub tod : t(3)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Joyb(1)
Next
Gosub tod : t(4)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Joyr(1)
Next
Gosub tod : t(5)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=RawStatus($45)
Next
Gosub tod : t(6)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=Inkey$
Next
Gosub tod : t(7)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=MouseX
Next
Gosub tod : t(8)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=MouseY
Next
Gosub tod : t(9)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=MouseXSpeed
Next
Gosub tod : t(10)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Sound 0,1
Next
Gosub tod : t(11)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Volume 1,32
Next
Gosub tod : t(12)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
LoopSound 0,1
Next
Gosub tod : t(13)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "joyx ",t(2)," 2000 1"
NPrint "joyy ",t(3)," 2000 1"
NPrint "joyb ",t(4)," 2000 1"
NPrint "joyr ",t(5)," 2000 1"
NPrint "rawstatus ",t(6)," 2000 1"
NPrint "inkey ",t(7)," 2000 1"
NPrint "mousex ",t(8)," 2000 1"
NPrint "mousey ",t(9)," 2000 1"
NPrint "mousexspeed ",t(10)," 2000 1"
NPrint "sound ",t(11)," 2000 1"
NPrint "volume ",t(12)," 2000 1"
NPrint "loopsound ",t(13)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return

