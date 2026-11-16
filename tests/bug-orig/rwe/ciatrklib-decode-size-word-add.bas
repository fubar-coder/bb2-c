; ciatrklib-decode-size-word-add (#434) - DecodeModule copies a module
; from fast memory to chip memory with the size computed from the
; patterns and the sample lengths + 3, added as a word (ADD #3,d0): for a
; size with a low word of $FFFD-$FFFF the carry is lost and the copy is
; 64 KB short.  The module (built in fast memory) has one pattern and one
; sample of 64481 words: 1084 + 1024 + 128962 = $1FFFE bytes.  It ends
; with the long $CAFEBABE.
; Correct: the length is $1FFFE + 3 (131073) and the copy ends with
; $CAFEBABE.
; @fixed #434
; @rwe ciatrklib
DEFTYPE .l
MOVE.l #131088,d0 : MOVE.l #$10004,d1 : MOVE.l 4,a6 : JSR -198(a6) : PutReg d0,m
NPrint "allocated ",m<>0
If m
  Poke.b m+950,1
  Poke.l m+$438,$4D2E4B2E
  Poke.w m+42,64481
  Poke.l m+$1FFFA,$CAFEBABE
  DecodeModule 0,m
  l=GetTrackerLocation(0)
  NPrint "copied ",l<>m
  NPrint "size ",GetTrackerSize(0)
  NPrint "end ",Peek.l(l+$1FFFA)=$CAFEBABE
  FreeTrackerModule 0
  GetReg d0,m : MOVE.l d0,a1 : MOVE.l #131088,d0 : MOVE.l 4,a6 : JSR -210(a6)
EndIf
End
