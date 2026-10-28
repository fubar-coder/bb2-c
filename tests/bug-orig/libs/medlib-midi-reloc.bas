; medlib-midi-reloc (#207) - _RelocModule keeps the address of the
; expansion data in d0, but relocmdd (MIDI dumps) counts the dumps in the
; low word of d0: afterwards the "next module" pointer is read from the
; expansion address AND $FFFF0000, and a long there that is not 0 is
; relocated as a further module.  The module here (generated) has one
; MIDI dump.  So that the long read is one the program controls, it puts
; the module into chip memory itself, 64 bytes behind a 64K boundary
; (DecodeMedModule uses a module in chip memory in place): at the boundary
; a "module" whose next pointer points to itself and whose song pointer
; (+8) is 4; relocating it adds the module address to the 4.
; StartMedModule relocates.
; Correct: the next module pointer is read from the expansion data (0) -
; the long at boundary+8 stays 4.
; @configs clib
; @pass file=midi.med|@TESTDATA@/medlib-midi-reloc.med
DEFTYPE .l
k=AllocMem_(200000,$10002)
NPrint "chip memory ",k<>0
If k=0 Then End
b=(k+$FFFF) AND $FFFF0000
a=b+64
Poke.l b,b
Poke.l b+8,4
If ReadFile(0,"CORPUS:midi.med")
  ReadMem 0,a,4096
  CloseFile 0
EndIf
DecodeMedModule 0,a
NPrint "in place ",Peek.l(Addr MedModule(0)+8)=a
StartMedModule 0
NPrint "id ",Left$(Peek$(a),4)," boundary+8 relocated ",Peek.l(b+8)<>4
StopMed
FreeMem_ k,200000
End
