; bcmp - perf benchmark: compiled comparisons with a default-typed (Quick) variable, #1177
Dim t.l(9)
t0.l=0 : tt.l=0 : th.l=0 : tl.l=0 : t2.l=0 : i.l=0 : c.l=0
cnt.l=999 : cq.l=1000 : l.l=5 : x=1000.5 : y=1000 : n=8 : w=2000
VWait 5
BLITZ
Poke.b $bfde00,0 : Poke.b $bfdf00,0
Poke.b $bfd400,99 : Poke.b $bfd500,0
Poke.b $bfd600,$ff : Poke.b $bfd700,$ff
Poke.b $bfdf00,$51 : Poke.b $bfde00,$11
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
If cnt<x Then c=1
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
If x>cnt Then c=1
Next
Gosub tod : t(3)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
If cq=y Then c=1
Next
Gosub tod : t(4)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
If n/2=Int(n/2) Then c=1
Next
Gosub tod : t(5)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
If x>1 Then c=1
Next
Gosub tod : t(6)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
If l>1 Then c=1
Next
Gosub tod : t(7)=(tt-t0)&65535
cnt=0
Gosub tod : t0=tt
While cnt<w
cnt+1
Wend
Gosub tod : t(8)=(tt-t0)&65535
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "ltq ",t(2)," 2000 1"
NPrint "qgtl ",t(3)," 2000 1"
NPrint "leqq ",t(4)," 2000 1"
NPrint "inteq ",t(5)," 2000 1"
NPrint "qgtconst ",t(6)," 2000 1"
NPrint "lgtconst ",t(7)," 2000 1"
NPrint "whilelq ",t(8)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return
