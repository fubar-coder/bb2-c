; bproc - perf benchmark: a Statement and a Function with a string parameter and a local string
; (an own program, not part of bstr: every call allocates the local string with exec's AllocMem
; and frees it again, and AllocMem walks a free list whose shape depends on what the program has
; allocated before; in bstr, any loop added elsewhere moved these two by 2 % for the port and
; the original alike. Here nothing but this file moves them: keep the setup as it is, and
; re-record their targets (perf-original) with any change to it.)
DEFTYPE .l
Dim t(3)
c=0
a$="Hello, World of Blitz"
Statement pstr{s$}
u$=s$
End Statement
Function.l fstr{s$}
u$=s$
Function Return Len(u$)
End Function
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
pstr{a$}
Next
Gosub tod : t(2)=(tt-t0)&65535
Gosub tod : t0=tt
For i=1 To 2000
c=fstr{a$}
Next
Gosub tod : t(3)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "pstr ",t(2)," 2000 1"
NPrint "fstr ",t(3)," 2000 1"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return
