; bq48 - perf benchmark: compiled expressions that rule P+ computes in 48.16 or by the routines of #1178
; (expr-tree-5-operand-typing, design decision 9, task 4.3): a Long beside a Quick, l narrow (100) and lx
; wide (100000), the wanted types Long (r) and Quick (x), If, Select and Mod with a fraction, a Quick
; with a constant divisor or factor
Dim t.l(21)
t0.l=0 : tt.l=0 : th.l=0 : tl.l=0 : t2.l=0 : i.l=0 : c.l=0 : r.l=0 : x.q=0
l.l=100 : lx.l=100000 : q.q=2.5 : p.q=1.25 : w.w=3 : qw.q=4
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
r=l*q
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=lx*q
Next
Gosub tod : t(3)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=q*l
Next
Gosub tod : t(4)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
x=l*q
Next
Gosub tod : t(5)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
x=lx*q
Next
Gosub tod : t(6)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=l+q+p
Next
Gosub tod : t(7)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
x=l+q+p
Next
Gosub tod : t(8)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=q*p+l
Next
Gosub tod : t(9)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=q*w+p
Next
Gosub tod : t(10)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
If l+q>p Then c=1
Next
Gosub tod : t(11)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
If l*q>p Then c=1
Next
Gosub tod : t(12)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
If q<l Then c=1
Next
Gosub tod : t(13)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Select l+q
Case 1
c=1
Case 2
c=2
Case 3
c=3
Default
c=4
End Select
Next
Gosub tod : t(14)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
x=q Mod 1.5
Next
Gosub tod : t(15)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
x=l Mod q
Next
Gosub tod : t(16)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
x=l/q
Next
Gosub tod : t(17)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r=q/4+l
Next
Gosub tod : t(18)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
x=q*0.5+l
Next
Gosub tod : t(19)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
x=l/qw
Next
Gosub tod : t(20)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "lmulq ",t(2)," 2000 1"
NPrint "lxmulq ",t(3)," 2000 1"
NPrint "qmull ",t(4)," 2000 1"
NPrint "xlmulq ",t(5)," 2000 1"
NPrint "xlxmulq ",t(6)," 2000 1"
NPrint "laddqp ",t(7)," 2000 1"
NPrint "xladdqp ",t(8)," 2000 1"
NPrint "qmulpaddl ",t(9)," 2000 1"
NPrint "qmulwaddp ",t(10)," 2000 1"
NPrint "ifladdq ",t(11)," 2000 1"
NPrint "iflmulq ",t(12)," 2000 1"
NPrint "ifqltl ",t(13)," 2000 1"
NPrint "selladdq ",t(14)," 2000 1"
NPrint "xqmodc ",t(15)," 2000 1"
NPrint "xlmodq ",t(16)," 2000 1"
NPrint "xldivq ",t(17)," 2000 1"
NPrint "qdiv4addl ",t(18)," 2000 1"
NPrint "xqmulhaddl ",t(19)," 2000 1"
NPrint "xldivqw ",t(20)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return
