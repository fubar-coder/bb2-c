; rilesdebuglib-detoke-plane1-glyph (#709) - FPrintDetoke draws the name
; of a token into plane 0 and, with SetupLESDebug's tokenise, into plane 1
; as well.  With inverse printing (FInverse $100) the inverse drawing
; leaves the last glyph byte in the low byte of d0, the glyph offset that
; the plane 1 call takes: plane 1 gets the glyph of another character.
; The program builds a token list of one token (1, "AB") and a line with
; that token in memory.
; Correct: plane 1 holds the same inverted characters as plane 0.
; @fixed #709
; @rwe rilesdebuglib
DEFTYPE .l
BitMap 0,640,16,2
t=AllocMem_(64,$10001)
If t=0 Then NPrint "no memory" : End
Poke.l t,0 : Poke.w t+4,1 : Poke$ t+6,"AB"
l=t+32 : Poke.b l,$80 : Poke.b l+1,1 : Poke.b l+2,0
SetupLESDebug t,1
FInverse $100
FLocate 0,0
FPrintDetoke l,100,0,0
FInverse 0
p0=Peek.l(Addr BitMap(0)+8) : p1=Peek.l(Addr BitMap(0)+12)
k=0
For i=0 To 7
  If Peek.b(p0+i*80)<>Peek.b(p1+i*80) OR Peek.b(p0+1+i*80)<>Peek.b(p1+1+i*80) Then k=1
Next
If k Then NPrint "plane 1 differs" Else NPrint "plane 1 the same"
FreeMem_ t,64
End
