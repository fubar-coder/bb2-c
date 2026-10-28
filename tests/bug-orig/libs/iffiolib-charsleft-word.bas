; iffiolib-charsleft-word (#177) - readbuff keeps only the low word of the
; dos Read result in charsleft: a read error (-1) counts as 65535 bytes
; read, and the old contents of the 2048 byte buffer are taken as the
; file's data.
; Here: LoadShape of an ILBM (16x4, 1 plane, BODY bytes 1..8) built in
; memory and written to RAM: fills the buffer; then LoadShape of PRT:,
; where Read returns -1 (port-handler, no printer).  The original loads
; the old ILBM from the buffer as shape 1 and the program goes on.
; Correct: the read error is an error (or the end of the file): there is
; no ILBM, LoadShape stops with a runtime error.
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
Statement pb{v.l}
  SHARED m,p
  Poke.b m+p,v : p=p+1
End Statement
Statement pw{v.l}
  pb{(v LSR 8) AND 255} : pb{v AND 255}
End Statement
Statement pl{v.l}
  pw{(v LSR 16) AND $FFFF} : pw{v AND $FFFF}
End Statement
Statement shinfo{s.l}
  a.l=Addr Shape(s)
  NPrint "shape ",s," ",Peek.w(a)," ",Peek.w(a+2)," ",Right$(Hex$(Peek.b(Peek.l(a+14)) AND 255),2)
End Statement
m=AllocMem(256,65536) : p=0
pl{$464F524D} : pl{0} : pl{$494C424D}
pl{$424D4844} : pl{20}
pw{16} : pw{4} : pw{0} : pw{0} : pb{1} : pb{0} : pb{0} : pb{0}
pw{0} : pb{10} : pb{11} : pw{320} : pw{200}
pl{$424F4459} : pl{8}
For i=1 To 8 : pb{i} : Next
Poke.l m+4,p-8
If WriteFile(0,"RAM:bo177.iff")
  WriteMem 0,m,p
  CloseFile 0
EndIf
FreeMem m,256
LoadShape 0,"RAM:bo177.iff"
shinfo{0}
KillFile "RAM:bo177.iff"
NPrint "load PRT:"
LoadShape 1,"PRT:"
shinfo{1}
End
