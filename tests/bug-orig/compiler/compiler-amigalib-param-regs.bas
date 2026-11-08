; compiler-amigalib-param-regs (#344) - an Amiga library function used
; inside an expression: blitz2 saves only the data registers covered by
; savereg (d0/d1) around the call, so a parameter loaded into d2 or higher
; overwrites an intermediate value of the expression held there.
; Write_ takes fh in d1, buf in d2, len in d3; in a+(b+(c+Write_(...))) c
; is in d2 when the call is made. BltBitMap_ takes d0-d7 (and a0-a2); in
; a+(b+(c+(d+(e+(f+BltBitMap_(...)))))) a to f are in d0-d5. Nested: a
; BltBitMap_ in a parameter of a Write_ in an expression. A string
; expression keeps the end of the string in a3: RawDoFmt_ takes putChData
; in a3. Correct: the program prints 1234 twice, 123456 plus the planes
; BltBitMap_ blits, 1233 plus them, and ab0. A wrong value depends on
; where the strings are, so the program prints only that it is wrong, not
; its value (the same text for every run and build).
; @fixed #344
fh.l=Open_("NIL:",1006)
s$="abcd"
a.l=1000:b.l=200:c.l=30
x.l=Write_(fh,&s$,4)
NPrint a+b+c+x
y.l=a+(b+(c+Write_(fh,&s$,4)))
If y=1234 Then NPrint y Else NPrint "wrong: not 1234"
m1.l=AllocBitMap_(16,2,1,1,0)
m2.l=AllocBitMap_(16,2,1,1,0)
p.l=BltBitMap_(m1,0,0,m2,0,0,16,1,$c0,1,0)
a=100000:b=20000:c=3000:d.l=400:e.l=50:f.l=6
y=a+(b+(c+(d+(e+(f+BltBitMap_(m1,0,0,m2,0,0,16,1,$c0,1,0))))))
If y=123456+p Then NPrint "123456+",p Else NPrint "wrong: not 123456+planes"
a=1000:b=200:c=30:d=1:e=2
y=a+(b+(c+Write_(fh,&s$,d+(e+BltBitMap_(m1,0,0,m2,0,0,16,1,$c0,1,0)))))
If y=1233+p Then NPrint "1233+",p Else NPrint "wrong: not 1233+planes"
f$="x":b$=String$(" ",16)
t$="ab"+Str$(RawDoFmt_(&f$,0,?putch,&b$) AND 0)
If t$="ab0" Then NPrint t$ Else NPrint "wrong: not ab0"
FreeBitMap_ m2
FreeBitMap_ m1
Close_ fh
End

putch:
  Dc.w $16C0,$4E75
