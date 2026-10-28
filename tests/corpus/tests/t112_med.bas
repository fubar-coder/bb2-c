; t112_med - medlib: LoadMedModule (MMD0 from the CD), StartMedModule
; (relocation "MMD0" -> "RMD0"), PlayMed called for every timing pulse
; (no interrupt: the program drives the player), GetMedNote/GetMedInstr/
; GetMedVolume (read and clear), JumpMed, SetMedVolume, SetMedMask,
; StopMed, DecodeMedModule (module copied into memory with ReadMem).
; Results: what the player reports per row, checksums over all pulses.
; The sample data is cleared before the start (the player then plays
; silence; the module is not relocated yet, so the sample offsets are
; relative to the module).
DEFTYPE .l
Function.l cks{a.l,n.l}
  c.l=0
  For i.l=0 To n-1 Step 2
    c=(c*3+(Peek.w(a+i) AND $FFFF)) AND $FFFFFF
  Next
  Function Return c
End Function
Statement silence{d.l}
  so.l=Peek.l(d+8) : sa.l=Peek.l(d+24)
  For i.l=0 To (Peek.b(d+so+787) AND 255)-1
    p.l=Peek.l(d+sa+4*i)
    If p
      For a.l=d+p+6 To d+p+5+Peek.l(d+p) : Poke.b a,0 : Next
    EndIf
  Next
End Statement
Statement play{t$,n.l}
  c.l=0 : r$=""
  For k.l=1 To n
    PlayMed
    s$=""
    For ch.l=0 To 3
      nt=GetMedNote(ch) : ins=GetMedInstr(ch) : v=GetMedVolume(ch)
      c=(c*7+nt*4096+ins*64+v) AND $FFFFFF
      If nt Then s$=s$+Right$(Hex$(nt),2)+"/"+Right$(Hex$(ins),2)+"/"+Right$(Hex$(v),2)+" " Else s$=s$+"-------- "
    Next
    If k<=48 AND (k MOD 6)=1 Then NPrint t$," ",k," ",s$
  Next
  NPrint t$," sum ",Hex$(c)
End Statement
f$="BB2:examples/amigamode/lambada.med"
LoadMedModule 0,f$
o=Addr MedModule(0)
d=Peek.l(o+8) : l=Peek.l(o+4)
NPrint "loaded id ",Hex$(Peek.l(o))," len ",l," data ",d<>0," same id ",Peek.l(d)=Peek.l(o)
NPrint "cks ",Hex$(cks{d,l})
silence{d}
NPrint "silenced cks ",Hex$(cks{d,l})
StartMedModule 0
Print "started id ",Hex$(Peek.l(d))," pstate ",Hex$(Peek.w(d+40))," block ",Peek.w(d+42)
NPrint " line ",Peek.w(d+44)," seq ",Peek.w(d+46)
play{"a",400}
NPrint "pos block ",Peek.w(d+42)," line ",Peek.w(d+44)," seq ",Peek.w(d+46)," counter ",Peek.b(d+50)
JumpMed 3
play{"j",60}
NPrint "pos block ",Peek.w(d+42)," line ",Peek.w(d+44)," seq ",Peek.w(d+46)
SetMedVolume 32
play{"v",60}
SetMedMask 5
play{"m",60}
SetMedMask 15
StopMed
NPrint "stopped pstate ",Peek.w(d+40)
PlayMed
NPrint "after stop ",GetMedNote(0)," ",GetMedInstr(0)
; DecodeMedModule from memory (not chip: copied)
If ReadFile(0,f$)
  Dim mm.l(l/4+2)
  ReadMem 0,&mm(0),l
  CloseFile 0
  silence{&mm(0)}
  DecodeMedModule 1,&mm(0)
  o=Addr MedModule(1)
  NPrint "decoded id ",Hex$(Peek.l(o))," len ",Peek.l(o+4)," copy ",Peek.l(o+8)<>&mm(0)," cks ",Hex$(cks{Peek.l(o+8),l})
  StartMedModule 1
  play{"d",120}
  StopMed
EndIf
NPrint "end"
End
