; t09_memacc - memacclib: Poke, Peek, Peeks$, Call
Dim m.l(31)
a.l=&m(0)
For i=0 To 31 : m(i)=0 : Next
Poke.b a,$81 : Poke.b a+1,300 : Poke.w a+2,-2 : Poke.l a+4,$12345678
NPrint "peek.b ",Peek.b(a)," ",Peek.b(a+1)
NPrint "peek.w ",Peek.w(a)," ",Peek.w(a+2)
NPrint "peek.l ",Peek.l(a)," ",Peek.l(a+4)
NPrint "peek ",Peek(a+4)
Poke a+8,-77 : NPrint "poke ",Peek.l(a+8)," ",Peek.w(a+10)
q.q=1.5 : Poke.q a+12,q : NPrint "quick ",Peek.q(a+12)," ",Peek.l(a+12)
f.f=-2.25 : Poke.f a+16,f : NPrint "float ",Peek.f(a+16)," ",Hex$(Peek.l(a+16))
Poke.s a+20,"Hello, world" : NPrint "peek$ [",Peek$(a+20),"] ",Len(Peek$(a+20))
Poke.s a+40,"" : NPrint "empty [",Peek$(a+40),"] ",Len(Peek$(a+40))," ",Peek.b(a+40)
b$="abc"+Chr$(0)+"def" : Poke.s a+48,b$
NPrint "embedded0 [",Peek$(a+48),"] ",Peek.b(a+51)," ",Peek.b(a+52)," ",Peek.b(a+55)
NPrint "peeks$ [",Peeks$(a+20,5),"] [",Peeks$(a+20,0),"] ",Len(Peeks$(a+20,7))
c$=Peeks$(a+20,3)+"|"+Peeks$(a+27,5)
NPrint "concat [",c$,"]"
NPrint "peeks$ 0 inside [",Peeks$(a+48,7),"] ",Len(Peeks$(a+48,7))
; Call: moveq #42,d0 / move.l d0,abs.l / rts
Poke.w a+64,$702A : Poke.w a+66,$23C0 : Poke.l a+68,a+80 : Poke.w a+72,$4E75
Poke.l a+80,0
Call a+64
NPrint "call ",Peek.l(a+80)
Call a+72
NPrint "call rts ok"
x.l=Peek.l(a+4)+Peek.w(a+2)*2+Peek.b(a)
NPrint "expr ",x
End
