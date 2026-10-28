; palettelib-make4bitrgbs-lowbyte (#224) - make4bitrgbs (CD binary) builds
; the 4 bit colours for LoadRGB4 from the LOW byte of each 32 bit
; component, but DecodePalette sets only the top byte: under Kickstart
; < 3.0 (agamode 0, ShowPalette with LoadRGB4) such colours are black.
; agamode comes from ExecVersion when palettelib starts.  Here the program
; (CORPUS:out/ID, where the runner puts it) runs itself a second time (dos
; Execute, output to the same file) with the version of exec.library set
; to 37 for that run, then puts it back.
; The child opens a screen, decodes a CMAP (bytes i*21+5) and shows it;
; Red/Green/Blue (GetRGB4) of colours 1-3 are 0 in the original.
; Correct: the top 4 bits of each component: 4,5,6 8,9,10 12,13,14.
; @configs clib
DEFTYPE .l
If NumPars=0
  e=Peek.l(4) : v=Peek.w(e+20)
  c$="CORPUS:out/palettelib-make4bitrgbs-lowbyte child"
  Poke.w e+20,37
  Execute_ &c$,0,Output_
  Poke.w e+20,v
  NPrint "back ",Peek.w(e+20)=v
  End
EndIf
NPrint "exec ",ExecVersion
Screen 0,0,0,320,200,3,0,"bo224",1,2
Dim mm.l(20) : a=&mm(0)
Poke.l a,$464F524D : Poke.l a+4,24 : Poke.l a+8,$494C424D
Poke.l a+12,$434D4150 : Poke.l a+16,12
For i=0 To 11 : Poke.b a+20+i,i*21+5 : Next
DecodePalette 2,a : ShowPalette 2
Print "cols"
For i=1 To 3 : Print " ",Red(i),",",Green(i),",",Blue(i) : Next
NPrint ""
CloseScreen 0
End
