; bmath - perf benchmark: arithmetic, Mod, Quick/Float conversion, a procedure, Rnd, Sgn, Abs, Int, Frac, QLimit, Peek, Poke
; (Int and Frac of a positive and of a negative Quick; Sgn and Abs of a Quick and
; Peek of a constant address, the candidates of compiler-intrinsics)
; (mixmul to mixmulh, qqmul to qqmulc, lqdiv, lqdivw: a Quick under a Long type so far, compilerlib, #1178, #1191)
DEFTYPE .l
Dim t(56)
a=123456 : b=789 : c=0 : a2=123456789 : b2=70000 : a4=100 : b4=20
q1.q=12.5 : q2.q=3.25 : q3.q=0
q4.q=1000.5 : q5.q=1.0000153 : q6.q=-12.25 : a3=123456789 : b3=65536
q7.q=3
f1.f=12.5 : f2.f=3.25 : f3.f=0
w1.w=1234 : w2.w=57 : w3.w=0
Statement plocal{x.l}
y.l=x+1 : z.l=y
End Statement
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
Gosub tod : t0=tt
For i=1 To 2000
q3=q4/q5
Next
Gosub tod : t(34)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=a3/b3
Next
Gosub tod : t(35)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
w3=w1 MOD w2
Next
Gosub tod : t(36)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=q1 MOD q2
Next
Gosub tod : t(37)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=f1 MOD f2
Next
Gosub tod : t(38)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
f3=q1
Next
Gosub tod : t(39)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=f1
Next
Gosub tod : t(40)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
plocal{a}
Next
Gosub tod : t(41)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=a4*b4
Next
Gosub tod : t(42)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=Int(q6)
Next
Gosub tod : t(43)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=Frac(q6)
Next
Gosub tod : t(44)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Sgn(q1)
Next
Gosub tod : t(45)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
q3=Abs(q6)
Next
Gosub tod : t(46)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Peek.l(4)
Next
Gosub tod : t(47)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=a*q2
Next
Gosub tod : t(48)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=a4/q2
Next
Gosub tod : t(49)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=a4/q7
Next
Gosub tod : t(50)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=b4*q2
Next
Gosub tod : t(51)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=q7*4
Next
Gosub tod : t(52)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=a*q7
Next
Gosub tod : t(53)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=q1*q2
Next
Gosub tod : t(54)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=q1*q7
Next
Gosub tod : t(55)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=q7*q7
Next
Gosub tod : t(56)=(tt-t0)&65535
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
NPrint "qdivw ",t(34)," 2000 1"
NPrint "ldivw ",t(35)," 2000 1"
NPrint "wmod ",t(36)," 2000 1"
NPrint "qmod ",t(37)," 2000 1"
NPrint "fmod ",t(38)," 2000 1"
NPrint "q2f ",t(39)," 2000 1"
NPrint "f2q ",t(40)," 2000 1"
NPrint "plocal ",t(41)," 2000 1"
NPrint "lmuls ",t(42)," 2000 1"
NPrint "intn ",t(43)," 2000 1"
NPrint "fracn ",t(44)," 2000 1"
NPrint "sgnq ",t(45)," 2000 1"
NPrint "absq ",t(46)," 2000 1"
NPrint "peekc ",t(47)," 2000 1"
NPrint "mixmul ",t(48)," 2000 1"
NPrint "lqdiv ",t(49)," 2000 1"
NPrint "lqdivw ",t(50)," 2000 1"
NPrint "mixmulw ",t(51)," 2000 1"
NPrint "mixmulc ",t(52)," 2000 1"
NPrint "mixmulh ",t(53)," 2000 1"
NPrint "qqmul ",t(54)," 2000 1"
NPrint "qqmulw ",t(55)," 2000 1"
NPrint "qqmulc ",t(56)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return

