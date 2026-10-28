; gtlib-button-toggle-null (#171) - GTButton with the toggle flag (bit
; 13) sets TOGGLESELECT (BSET #0,14(a0)) in the new gadget without
; checking the result of CreateGadget: when it fails, bit 0 of address 14
; is set.  CreateGadget fails here because the previous gadget of the
; GTList is 0 - the state after an earlier failed gadget (gtlib stores the
; result unchecked as the previous one), set with a Poke.  Bit 0 of
; address 14 is cleared before and restored after.
; Correct: no gadget, address 14 unchanged.
; @configs clib
WbToScreen 0
GTButton 0,1,10,10,60,12,"one",0
a.l=Addr GTList(0)
NPrint "first ",Peek.l(a+8)<>0
Poke.l a+8,0
b.b=Peek.b(14)
Poke.b 14,b AND $FE
GTButton 0,2,10,30,60,12,"two",$2000
c.b=Peek.b(14)
Poke.b 14,b
NPrint "second ",Peek.l(a+8)<>0
If c=(b AND $FE)
  NPrint "address 14 unchanged"
Else
  NPrint "address 14 changed"
EndIf
End
