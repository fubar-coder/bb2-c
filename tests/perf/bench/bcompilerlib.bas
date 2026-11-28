; bcompilerlib - perf benchmark: the 48.16 routines of compilerlib ($D603-$D60E, expr-tree-5-operand-typing,
; design decision 9) and its conversions ($D612-$D614: q48sat, f2l, f2q; type-system-rework task 4.3),
; called from inline assembler with SysJsr as the compiler will call them: a in d0/d1 (low, high), b in
; d2/d3, a Float in d0; load: the register loads alone (to subtract for the routine alone)
Dim t.l(23)
t0.l=0 : tt.l=0 : th.l=0 : tl.l=0 : t2.l=0 : i.l=0
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
For i=1 To 2000
MOVE.l #$18000,d0 : MOVEQ #0,d1 : MOVE.l #$28000,d2 : MOVEQ #0,d3
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$18000,d0 : MOVEQ #0,d1 : MOVE.l #$28000,d2 : MOVEQ #0,d3 : SysJsr $D603
Next
Gosub tod : t(3)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$18000,d0 : MOVEQ #0,d1 : MOVE.l #$28000,d2 : MOVEQ #0,d3 : SysJsr $D605
Next
Gosub tod : t(4)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$18000,d0 : MOVEQ #0,d1 : MOVE.l #$30000,d2 : MOVEQ #0,d3 : SysJsr $D605
Next
Gosub tod : t(5)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$86A08000,d0 : MOVEQ #1,d1 : MOVE.l #$28000,d2 : MOVEQ #0,d3 : SysJsr $D605
Next
Gosub tod : t(6)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$86A08000,d0 : MOVEQ #1,d1 : MOVE.l #$86A08000,d2 : MOVEQ #1,d3 : SysJsr $D605
Next
Gosub tod : t(7)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$18000,d0 : MOVEQ #0,d1 : MOVE.l #$28000,d2 : MOVEQ #0,d3 : SysJsr $D606
Next
Gosub tod : t(8)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$86A08000,d0 : MOVEQ #1,d1 : MOVE.l #$28000,d2 : MOVEQ #0,d3 : SysJsr $D606
Next
Gosub tod : t(9)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$86A08000,d0 : MOVEQ #1,d1 : MOVE.l #$86A00000,d2 : MOVEQ #1,d3 : SysJsr $D606
Next
Gosub tod : t(10)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$18000,d0 : MOVEQ #0,d1 : MOVE.l #$30000,d2 : MOVEQ #0,d3 : SysJsr $D606
Next
Gosub tod : t(11)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$86A08000,d0 : MOVEQ #1,d1 : MOVE.l #$86A08000,d2 : MOVEQ #1,d3 : SysJsr $D606
Next
Gosub tod : t(12)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$78000,d0 : MOVEQ #0,d1 : MOVE.l #$20000,d2 : MOVEQ #0,d3 : SysJsr $D60A
Next
Gosub tod : t(13)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #100000,d0 : SysJsr $D60B
Next
Gosub tod : t(14)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #-$18000,d0 : SysJsr $D60C
Next
Gosub tod : t(15)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$86A08000,d0 : MOVEQ #1,d1 : SysJsr $D60D
Next
Gosub tod : t(16)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$FFFE8000,d0 : MOVEQ #-1,d1 : SysJsr $D60E
Next
Gosub tod : t(17)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$18000,d0 : MOVEQ #0,d1 : SysJsr $D612
Next
Gosub tod : t(18)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$86A08000,d0 : MOVEQ #1,d1 : SysJsr $D612
Next
Gosub tod : t(19)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$A00000C2,d0 : SysJsr $D613
Next
Gosub tod : t(20)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$9502F962,d0 : SysJsr $D613
Next
Gosub tod : t(21)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$E66666C4,d0 : SysJsr $D614
Next
Gosub tod : t(22)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
MOVE.l #$80000050,d0 : SysJsr $D614
Next
Gosub tod : t(23)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "load ",t(2)," 2000 1"
NPrint "q48add ",t(3)," 2000 1"
NPrint "q48mul ",t(4)," 2000 1"
NPrint "q48mulw ",t(5)," 2000 1"
NPrint "q48mulx ",t(6)," 2000 1"
NPrint "q48mulxx ",t(7)," 2000 1"
NPrint "q48div ",t(8)," 2000 1"
NPrint "q48divx ",t(9)," 2000 1"
NPrint "q48divl ",t(10)," 2000 1"
NPrint "q48divw ",t(11)," 2000 1"
NPrint "q48divxx ",t(12)," 2000 1"
NPrint "q48mod ",t(13)," 2000 1"
NPrint "l2q48 ",t(14)," 2000 1"
NPrint "q2q48 ",t(15)," 2000 1"
NPrint "q48floor ",t(16)," 2000 1"
NPrint "q48trunc ",t(17)," 2000 1"
NPrint "q48sat ",t(18)," 2000 1"
NPrint "q48satx ",t(19)," 2000 1"
NPrint "f2l ",t(20)," 2000 1"
NPrint "f2lx ",t(21)," 2000 1"
NPrint "f2q ",t(22)," 2000 1"
NPrint "f2qx ",t(23)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return
