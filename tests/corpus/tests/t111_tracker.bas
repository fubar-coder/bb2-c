; t111_tracker - trackerlib: LoadModule (NoiseTracker module from the CD),
; PlayModule (mt_init clears the first long of every sample, filter off;
; mt_music then runs in the vertical blank interrupt), StopModule, freeing
; a playing module by loading another one.  Audio output is not checked;
; the sample data is cleared before every PlayModule (silence), every
; PlayModule is stopped at once.
DEFTYPE .l
Function.l cks{a.l,n.l}
  c.l=0
  For i.l=0 To n-1 Step 2
    c=(c*3+(Peek.w(a+i) AND $FFFF)) AND $FFFFFF
  Next
  Function Return c
End Function
; clear the samples behind the patterns (highest pattern from the song)
Statement silence{d.l,l.l}
  mx.l=0
  For i.l=0 To 127
    p.l=Peek.b(d+952+i) AND 255
    If p>mx Then mx=p
  Next
  For a.l=d+1084+(mx+1)*1024 To d+l-1 : Poke.b a,0 : Next
End Statement
f$="BB2:bum9a/bum9examples2/hyperhelp/hhmusic.mus"
LoadModule 0,f$
m=Addr Module(0)
d=Peek.l(m) : l=Peek.l(m+4)
NPrint "loaded ",d<>0," len ",l," id ",Hex$(Peek.l(d+1080))
NPrint "song len ",Peek.b(d+950)," restart ",Peek.b(d+951)," cks ",Hex$(cks{d,l})
silence{d,l}
NPrint "silenced cks ",Hex$(cks{d,l})
Filter 1
PlayModule 0
NPrint "playing cks ",Hex$(cks{d,l})," filter ",Peek.b($BFE001) AND 2
StopModule
NPrint "stopped cks ",Hex$(cks{d,l})
PlayModule 0
StopModule
; loading into the playing object stops it and frees the old data
LoadModule 0,f$
NPrint "reloaded ",Peek.l(Addr Module(0))<>0," len ",Peek.l(Addr Module(0)+4)," cks ",Hex$(cks{Peek.l(Addr Module(0)),l})
LoadModule 1,f$
silence{Peek.l(Addr Module(1)),Peek.l(Addr Module(1)+4)}
PlayModule 1
StopModule
StopModule
NPrint "end"
End
