; bstr - perf benchmark: strings, Read, lists, Sort (with Len and Asc of an
; element and a field, Instr of two variables and Cvi: the candidates of
; compiler-intrinsics)
DEFTYPE .l
NEWTYPE .rec
  id.w
  name$
End NEWTYPE
Dim t(37)
a=123456 : c=0
a$="Hello, World of Blitz" : b$="Hello, World of Blitz" : n$="12345"
r$=String$(" ",80)
Dim List ll.l(100)
For i=1 To 50
If AddItem(ll()) Then ll()=i
Next
Dim sa.l(200)
For i=0 To 200 : sa(i)=i*3 : Next
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
r$=a$
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=Mid$(a$,3,5)
Next
Gosub tod : t(3)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=Left$(a$,5)
Next
Gosub tod : t(4)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=Right$(a$,5)
Next
Gosub tod : t(5)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=Str$(a)
Next
Gosub tod : t(6)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Val(n$)
Next
Gosub tod : t(7)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Len(a$)
Next
Gosub tod : t(8)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=Chr$(65)
Next
Gosub tod : t(9)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Asc(a$)
Next
Gosub tod : t(10)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Instr(a$,"Blitz")
Next
Gosub tod : t(11)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=UCase$(a$)
Next
Gosub tod : t(12)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=Hex$(a)
Next
Gosub tod : t(13)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=a$+b$
Next
Gosub tod : t(14)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
If a$=b$ Then c=1
Next
Gosub tod : t(15)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=String$("a",20)
Next
Gosub tod : t(16)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
Restore dd : Read c,c,c,c
Next
Gosub tod : t(17)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 500
ResetList ll()
While NextItem(ll())
Wend
Next
Gosub tod : t(18)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
ResetList ll()
If AddItem(ll()) Then KillItem ll()
Next
Gosub tod : t(19)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
ResetList ll()
Next
Gosub tod : t(20)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 50
Sort sa()
Next
Gosub tod : t(21)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=Bin$(a)
Next
Gosub tod : t(22)=(tt-t0)&65535
n$="The quick brown fox jumps over the lazy dog, then naps again"
Gosub tod : t0=tt
For i=1 To 2000
r$=Mid$(n$,3,40)
Next
Gosub tod : t(23)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=Right$(n$,40)
Next
Gosub tod : t(24)=(tt-t0)&65535
s1$=String$("a",10) : s2$=String$("a",100) : s3$=String$("a",1000)
Gosub tod : t0=tt
For i=1 To 2000
c=Len(s1$)
Next
Gosub tod : t(25)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Len(s2$)
Next
Gosub tod : t(26)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Len(s3$)
Next
Gosub tod : t(27)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Asc(s1$)
Next
Gosub tod : t(28)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Asc(s2$)
Next
Gosub tod : t(29)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Asc(s3$)
Next
Gosub tod : t(30)=(tt-t0)&65535
Dim s$(5) : s$(3)="Hello, World of Blitz" : rr.rec\name="Hello, World of Blitz"
f$="Blitz" : m$=Mki$(1234)
Gosub tod : t0=tt
For i=1 To 2000
c=Len(s$(3))
Next
Gosub tod : t(31)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Asc(s$(3))
Next
Gosub tod : t(32)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Len(rr\name)
Next
Gosub tod : t(33)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Asc(rr\name)
Next
Gosub tod : t(34)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Instr(a$,f$)
Next
Gosub tod : t(35)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=Cvi(m$)
Next
Gosub tod : t(36)=(tt-t0)&65535
Goto skipdata
dd:
Data.l 1,2,3,4
skipdata:
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "asg_s ",t(2)," 2000 1"
NPrint "mid ",t(3)," 2000 1"
NPrint "left ",t(4)," 2000 1"
NPrint "right ",t(5)," 2000 1"
NPrint "str ",t(6)," 2000 1"
NPrint "val ",t(7)," 2000 1"
NPrint "len ",t(8)," 2000 1"
NPrint "chr ",t(9)," 2000 1"
NPrint "asc ",t(10)," 2000 1"
NPrint "instr ",t(11)," 2000 1"
NPrint "ucase ",t(12)," 2000 1"
NPrint "hex ",t(13)," 2000 1"
NPrint "concat ",t(14)," 2000 1"
NPrint "strcmp ",t(15)," 2000 1"
NPrint "string ",t(16)," 2000 1"
NPrint "read4 ",t(17)," 2000 1"
NPrint "nextitem51 ",t(18)," 500 51"
NPrint "addkill ",t(19)," 2000 1"
NPrint "resetlist ",t(20)," 2000 1"
NPrint "sort201 ",t(21)," 50 1"
NPrint "bin ",t(22)," 2000 1"
NPrint "midlong ",t(23)," 2000 1"
NPrint "rightlong ",t(24)," 2000 1"
NPrint "len10 ",t(25)," 2000 1"
NPrint "len100 ",t(26)," 2000 1"
NPrint "len1000 ",t(27)," 2000 1"
NPrint "asc10 ",t(28)," 2000 1"
NPrint "asc100 ",t(29)," 2000 1"
NPrint "asc1000 ",t(30)," 2000 1"
NPrint "lenel ",t(31)," 2000 1"
NPrint "ascel ",t(32)," 2000 1"
NPrint "lenfld ",t(33)," 2000 1"
NPrint "ascfld ",t(34)," 2000 1"
NPrint "instrv ",t(35)," 2000 1"
NPrint "cvi ",t(36)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return

