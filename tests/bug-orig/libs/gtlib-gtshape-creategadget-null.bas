; gtlib-gtshape-creategadget-null (#327) - GTShape creates a button with
; CreateGadget and uses the result without checking it: if CreateGadget
; fails (0), it reads GadgetRender from address 18 and copies 20 bytes of
; its image to where that long points (or writes the image pointer to
; 18), and writes SelectRender, Flags, Activation and GadgetType to the
; addresses 12-25 - the exception vectors 3-6.
; Without a used screen (no Screen/WbToScreen) GetVisualInfo gets 0 and
; CreateGadget fails.  The program keeps the longs at 12-24, calls
; GTShape and compares them; then it puts the old values back.
; Correct: the vectors are unchanged.  The original changes them (or the
; copy to the address in them crashes the machine).
; @configs clib
DEFTYPE .l
Dim v.l(3)
InitShape 0,16,8,2
For i=0 To 3 : v(i)=Peek.l(12+i*4) : Next
GTShape 0,1,10,10,0,0
k=0
For i=0 To 3
  If Peek.l(12+i*4)<>v(i) Then k=k+1
  Poke.l 12+i*4,v(i)
Next
NPrint "gadget ",GTGadPtr(0,1)
NPrint "vectors changed ",k
End
