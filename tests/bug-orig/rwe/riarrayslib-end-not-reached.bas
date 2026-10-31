; riarrayslib-end-not-reached (#882) - FillArray stores an element and
; only then compares the pointer with the end of the array (BNE): an end
; that is not hit exactly runs on through all memory (the session ends in
; a timeout).  A Dim'med array always has an element, so the program
; empties one: it finds the array's header among the variables (the
; base pointer; the long behind it is the array's size in bytes, which
; the compiler adds to the base for the end it passes) and sets the size
; to 0 - an array without elements.
; Correct: FillArray on an array without elements writes nothing (the
; element at the start stays 0), the program goes on.
; @rwe riarrayslib
; @pass timeout=30
DEFTYPE .l
z=0
Dim a.l(99)
b=&a(0)
p=&z
h=0
For i=-32 To 32
  If h=0
    If Peek.l(p+i*2)=b
      If Peek.l(p+i*2+4)=400 Then h=p+i*2
    EndIf
  EndIf
Next
If h=0 Then NPrint "no header" : End
Poke.l h+4,0
NPrint "before"
FillArray a(),$12345678
NPrint "after"
NPrint a(0)
End
