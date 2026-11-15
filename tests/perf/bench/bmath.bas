; bmath - perf benchmark: Long, Quick and Float arithmetic, Rnd, Sgn, Abs, Int, Frac, QLimit, Peek, Poke
DEFTYPE .l
Dim t(34)
a=123456 : b=789 : c=0 : a2=123456789 : b2=70000
q1.q=12.5 : q2.q=3.25 : q3.q=0
f1.f=12.5 : f2.f=3.25 : f3.f=0
w1.w=1234 : w2.w=57 : w3.w=0
Dim buf.l(10)
p=&buf(0)
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
c=a
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=a*b
Next
Gosub tod : t(3)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=a/b
Next
Gosub tod : t(4)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=a MOD b
Next
Gosub tod : t(5)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
w3=w1*w2
Next
Gosub tod : t(6)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
w3=w1/w2
Next
Gosub tod : t(7)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=q1
Next
Gosub tod : t(8)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=q1*q2
Next
Gosub tod : t(9)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=q1/q2
Next
Gosub tod : t(10)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
f3=f1
Next
Gosub tod : t(11)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
f3=f1*f2
Next
Gosub tod : t(12)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
f3=f1/f2
Next
Gosub tod : t(13)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
f3=f1+f2
Next
Gosub tod : t(14)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=f1
Next
Gosub tod : t(15)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
f3=a
Next
Gosub tod : t(16)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=Rnd
Next
Gosub tod : t(17)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=Rnd(100)
Next
Gosub tod : t(18)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Sgn(a)
Next
Gosub tod : t(19)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Abs(a)
Next
Gosub tod : t(20)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=QAbs(q1)
Next
Gosub tod : t(21)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=Int(q1)
Next
Gosub tod : t(22)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=Frac(q1)
Next
Gosub tod : t(23)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=QLimit(q1,0,10)
Next
Gosub tod : t(24)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=QWrap(q1,0,10)
Next
Gosub tod : t(25)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Peek.b(p)
Next
Gosub tod : t(26)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Peek.w(p)
Next
Gosub tod : t(27)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Peek.l(p)
Next
Gosub tod : t(28)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Poke.b p,1
Next
Gosub tod : t(29)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Poke.w p,1
Next
Gosub tod : t(30)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Poke.l p,1
Next
Gosub tod : t(31)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=a2/b2
Next
Gosub tod : t(32)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=a2 MOD b2
Next
Gosub tod : t(33)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "asg_l ",t(2)," 2000 1"
NPrint "lmul ",t(3)," 2000 1"
NPrint "ldiv ",t(4)," 2000 1"
NPrint "lmod ",t(5)," 2000 1"
NPrint "wmul ",t(6)," 2000 1"
NPrint "wdiv ",t(7)," 2000 1"
NPrint "asg_q ",t(8)," 2000 1"
NPrint "qmul ",t(9)," 2000 1"
NPrint "qdiv ",t(10)," 2000 1"
NPrint "asg_f ",t(11)," 2000 1"
NPrint "fmul ",t(12)," 2000 1"
NPrint "fdiv ",t(13)," 2000 1"
NPrint "fadd ",t(14)," 2000 1"
NPrint "f2l ",t(15)," 2000 1"
NPrint "l2f ",t(16)," 2000 1"
NPrint "rnd ",t(17)," 2000 1"
NPrint "rndn ",t(18)," 2000 1"
NPrint "sgn ",t(19)," 2000 1"
NPrint "abs ",t(20)," 2000 1"
NPrint "qabs ",t(21)," 2000 1"
NPrint "int ",t(22)," 2000 1"
NPrint "frac ",t(23)," 2000 1"
NPrint "qlimit ",t(24)," 2000 1"
NPrint "qwrap ",t(25)," 2000 1"
NPrint "peekb ",t(26)," 2000 1"
NPrint "peekw ",t(27)," 2000 1"
NPrint "peekl ",t(28)," 2000 1"
NPrint "pokeb ",t(29)," 2000 1"
NPrint "pokew ",t(30)," 2000 1"
NPrint "pokel ",t(31)," 2000 1"
NPrint "ldiv32 ",t(32)," 2000 1"
NPrint "lmod32 ",t(33)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return

