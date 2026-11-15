; compiler-tokecode-stub-release (#1048) - a token jsr to a library
; routine with an error check, without runtime error checking.  TokeJsr
; (as ALibJsr in a library) calls gameiolib's Joyb through tokecode
; (blitz2.s:6047); Joyb's entry has an error check.  tokecode builds a
; stub `jsr Joyb / rts` for it whenever the entry has an error check,
; also without runtime error checking, where the check is never called:
; the jsr goes to the stub, one more JSR/RTS and 8 bytes for nothing.
; Correct: without runtime error checking the jsr goes straight to the
; routine.  The program counts the jsrs of the TokeJsr and those whose
; target starts with a jsr (the stub), and runs Joyb(1).
; @fixed #1048
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
