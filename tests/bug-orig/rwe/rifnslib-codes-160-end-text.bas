; rifnslib-codes-160-end-text (#657) - FNSPrint, FNSShadowPrint and
; FNSLength take a code from 160 on for the end of the text (a sign test
; after CMPI.b #32), and draw the codes the font has no glyph for (the
; built-in font and every known FNS font end at "z", 122) from the bytes
; behind the font.  Here: "Gr" + Chr$(252) + "e" against "Gr e" in the
; built-in font (FNSPrint into two bitmaps, FNSShadowPrint, FNSLength);
; "{" (123) against " " in a font of 91 glyphs installed with InstallFNS
; (built in memory, the bytes behind its last glyph set); and a font file
; of 2 glyphs (codes 32 and 33; the width of code 34 is 20) loaded with
; FNSLoad (BankLimit 80), measured with Chr$(34).
; Correct: a character the font has no glyph for is drawn and counted as
; a space (the width of code 32): every pair is the same.
; rifnslib needs RIAmosFuncLib (FNSLoad and FNSUnload use its banks).
; @fixed #657
; @rwe rifnslib riamosfunclib
; @pass timeout=60
DEFTYPE .l
f=AllocMem_(4096,$10001)
m=AllocMem_(512,$10001)
If f=0 OR m=0 Then NPrint "no memory" : End
BitMap 0,320,32,1
BitMap 1,320,32,1
FNSOutput 0
FNSClip 0,0,319,31
Gosub clear
FNSPrint 0,10,10,"Gr"+Chr$(252)+"e",0,1
FNSOutput 1
FNSPrint 0,10,10,"Gr e",0,1
Gosub compare : NPrint "print: ",r$
Gosub clear
FNSOutput 0
FNSShadowPrint 0,10,10,"Gr"+Chr$(252)+"e",0,1,1
FNSOutput 1
FNSShadowPrint 0,10,10,"Gr e",0,1,1
Gosub compare : NPrint "shadow: ",r$
a=FNSLength(0,"Gr"+Chr$(252)+"e",0)
b=FNSLength(0,"Gr e",0)
If a=b Then NPrint "length: same" Else NPrint "length: different"
; a font of 91 glyphs (codes 32 to 122), 16 x 8; behind it the bytes $FF
Poke.l f,$464E532E
Poke.w f+4,8 : Poke.w f+6,16 : Poke.w f+8,0 : Poke.w f+10,16
Poke.b f+32,6 : Poke.b f+123,8
For i=0 To 15 : Poke.b f+256+91*16+i,255 : Next
r=InstallFNS(1,f)
Gosub clear
FNSOutput 0
FNSPrint 1,10,10,"{",0,1
FNSOutput 1
FNSPrint 1,10,10," ",0,1
Gosub compare : NPrint "123: ",r$
; a font file of 2 glyphs (codes 32 and 33)
Poke.l m,$464E532E
Poke.w m+4,8 : Poke.w m+6,16 : Poke.w m+8,0 : Poke.w m+10,16
Poke.b m+32,5 : Poke.b m+33,7 : Poke.b m+34,20
If WriteFile(0,"RAM:bug-orig-two.fns")
  WriteMem 0,m,256+2*16
  CloseFile 0
EndIf
BankLimit 80
r=FNSLoad("RAM:bug-orig-two.fns",2)
a=FNSLength(2,"!"+Chr$(34),0)
b=FNSLength(2,"! ",0)
If a=b Then NPrint "load: same" Else NPrint "load: different"
FNSUnload 2
RemoveFNS 1
FreeMem_ m,512
FreeMem_ f,4096
End
compare:
p0=Peek.l(Addr BitMap(0)+8)
p1=Peek.l(Addr BitMap(1)+8)
r$="same"
For i=0 To 40*32-4 Step 4
  If Peek.l(p0+i)<>Peek.l(p1+i) Then r$="different"
Next
Return
clear:
p0=Peek.l(Addr BitMap(0)+8)
p1=Peek.l(Addr BitMap(1)+8)
For i=0 To 40*32-4 Step 4
  Poke.l p0+i,0 : Poke.l p1+i,0
Next
Return
