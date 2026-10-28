; t161_blitzkeys - blitzkeyslib in Amiga mode without key presses:
; BlitzQualifier after the init, RawStatus over all 128 raw codes (the
; key matrix of keyboard.device, read only), BlitzRepeat and
; HandleRawKey (no code).  BlitzKeys is Blitz mode only; BlitzKey takes
; entries out of keyboard.device's buffer (writes its read index), so it
; is not called here.
DEFTYPE .l
NPrint "qualifier ",BlitzQualifier
BlitzRepeat 10,2
HandleRawKey
n=0 : s=0
For k=0 To 127
  r=RawStatus(k)
  If r<>0 Then n=n+1 : s=s+k
  If r<>0 AND r<>-1 Then NPrint "odd ",k," ",r
Next
NPrint "pressed ",n," ",s
q.q=RawStatus($60) : NPrint "quick ",q
NPrint "qualifier ",BlitzQualifier
End
