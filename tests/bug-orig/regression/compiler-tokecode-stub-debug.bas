; compiler-tokecode-stub-debug (#1048) - a token jsr to a library
; routine with an error check, with runtime error checking.  TokeJsr
; calls gameiolib's Joyb through tokecode; with runtime error checking
; tokecode builds the stub that calls Joyb's error check before Joyb.
; Correct: the jsr goes to the stub (its target starts with the jsr of
; the check), and Joyb(1) runs.
; A regression test of the fix of #1048 (without runtime error checking
; the stub is left out).
; @pass runtime_errors=on
.tjstart
MOVEQ #1,d0
TokeJsr Joyb
.tjend
PutReg d0,b.w
NPrint "joyb ",b
s.l=?tjstart
e.l=?tjend
j.w=0
n.w=0
For p.l=s To e-6 Step 2
  If Peek.w(p)=$4EB9
    j+1
    t.l=Peek.l(p+2)
    If Peek.w(t)=$4EB9 Then n+1
  EndIf
Next
NPrint "jsrs ",j," stubs ",n
End
