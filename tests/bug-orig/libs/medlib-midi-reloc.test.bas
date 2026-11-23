; medlib-midi-reloc (#207) - _RelocModule keeps the address of the
; expansion data in d0, but relocmdd (MIDI dumps) counts the dumps in the
; low word of d0: afterwards the "next module" pointer is read from the
; expansion address AND $FFFF0000, and a long there that is not 0 is
; relocated as a further module.  The module here (generated) has one
; MIDI dump.  So that the long read is one the case controls, it puts
; the module into chip memory itself, 64 bytes behind a 64K boundary
; (DecodeMedModule uses a module in chip memory in place): at the boundary
; a "module" whose next pointer points to itself and whose song pointer
; (+8) is 4; relocating it adds the module address to the 4.
; StartMedModule relocates.
; Correct: the next module pointer is read from the expansion data (0) -
; the long at boundary+8 stays 4.
DEFTYPE .l

!TestCase{next_module}
  k=AllocMem_(200000,$10002)
  !Require{k<>0,"chip_memory"}
  b=(k+$FFFF) AND $FFFF0000
  a=b+64
  Poke.l b,b
  Poke.l b+8,4
  ok=ReadFile(0,"CORPUS:midi.med")
  !Require{ok<>0,"module_file"}
  ReadMem 0,a,4096
  CloseFile 0
  DecodeMedModule 0,a
  inplace=0
  If Peek.l(Addr MedModule(0)+8)=a Then inplace=1
  StartMedModule 0
  id$=Left$(Peek$(a),4)
  reloc=0
  If Peek.l(b+8)<>4 Then reloc=1
  StopMed
  FreeMem_ k,200000
  !CheckEq{inplace,1,"in_place"}
  !CheckStrEq{id$,"RMD1","id"}
  !CheckEq{reloc,0,"boundary_relocated"}
!EndTestCase
