; bstr - perf benchmark: strings, Read, lists, Sort, procedures with a string parameter
DEFTYPE .l
Dim t(25)
a=123456 : c=0
a$="Hello, World of Blitz" : b$="Hello, World of Blitz" : n$="12345"
r$=String$(" ",80)
Statement pstr{s$}
u$=s$
End Statement
Function.l fstr{s$}
u$=s$
Function Return Len(u$)
End Function
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
pstr{a$}
Next
Gosub tod : t(22)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=fstr{a$}
Next
Gosub tod : t(23)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
r$=Bin$(a)
Next
Gosub tod : t(24)=(tt-t0)&65535
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
NPrint "pstr ",t(22)," 2000 1"
NPrint "fstr ",t(23)," 2000 1"
NPrint "bin ",t(24)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return

