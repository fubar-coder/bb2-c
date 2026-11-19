; bfns - perf benchmark: FNSPrint and FNSLength of the RWE userlib
; rifnslib (the per-frame path of a game that prints a score or a
; message).  The built-in font (slot 0, 16 pixels wide, 8 rows), a text
; of 32 characters, into a 320 x 256 x 3 bitmap with the whole bitmap as
; the clip zone; the value is the cycles per call (32 characters).
; rifnslib calls RIAmosFuncLib (FNSLoad, FNSUnload).
; @rwe rifnslib riamosfunclib
DEFTYPE .l
Dim t(4)
c=0
t$="Score 0123456789 Lives 3 Level 7"
BitMap 0,320,256,3
FNSOutput 0,On
FNSInk 5
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
For i=1 To 1000
FNSPrint 0,8,100,t$
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=FNSLength(0,t$)
Next
Gosub tod : t(3)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "fnsprint ",t(2)," 1000 1"
NPrint "fnslength ",t(3)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return
