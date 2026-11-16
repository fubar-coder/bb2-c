; brec - perf benchmark: a recursive Statement with a string parameter, from depth 20 back to 0,
; repeated (an own program like bproc: every call allocates its list of local memory and the copy
; of the string parameter, and the cost of exec's AllocMem and FreeMem depends on the program's
; allocations before; keep the setup as it is, and re-record its target (the pass
; perf_brec.pass with the tag orig) with any change to it). The value is per call: one iteration
; is 21 calls, depth 1 to 21.
DEFTYPE .l
Dim t(2)
a$="Hello, World of Blitz"
Statement rec{n,s$}
If n>0 Then rec{n-1,s$}
End Statement
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
For i=1 To 100
rec{20,a$}
Next
Gosub tod : t(2)=(tt-t0)&65535
VWait 2
AMIGA
NPrint "empty ",t(0)," 20000 1"
NPrint "emptylong ",t(1)," 150000 1"
NPrint "rec20 ",t(2)," 100 21"
End
tod:
th=Peek.b($bfd700)&255
tl=Peek.b($bfd600)&255
t2=Peek.b($bfd700)&255
If t2<>th Then tl=Peek.b($bfd600)&255 : th=t2
tt=65535-((th LSL 8)+tl)
Return
