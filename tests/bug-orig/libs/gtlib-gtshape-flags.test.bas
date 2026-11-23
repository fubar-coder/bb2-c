; gtlib-gtshape-flags (#990) - GTShape turns bit 6 of its flags into the
; tag GA_Disabled and bit 7 into GA_Immediate for CreateGadget, and then
; overwrites the gadget's Flags with GFLG_GADGIMAGE and Activation with
; GACT_RELVERIFY, so both flags have no effect.
; Here: on the Workbench screen two GTShape gadgets, flags 64 and 128; the
; Flags word (offset 12) is tested for GFLG_DISABLED ($0100), the
; Activation word (offset 14) for GACT_IMMEDIATE (2).
; A GTButton with the same flags is the control (GadTools sets both).
; Correct: gadget 1 is disabled, gadget 2 immediate.
DEFTYPE .l

!TestCase{shape_flags}
  WbToScreen 0
  InitShape 0,16,8,2
  GTShape 0,1,10,10,64,0
  GTShape 0,2,10,30,128,0
  g=GTGadPtr(0,1)
  !Check{(Peek.w(g+12) AND $100)<>0,"flags_64_disabled"}
  g=GTGadPtr(0,2)
  !Check{(Peek.w(g+14) AND 2)<>0,"flags_128_immediate"}
  GTButton 0,3,10,50,80,14,"Ok",64
  GTButton 0,4,10,70,80,14,"Ok",128
  !Check{(Peek.w(GTGadPtr(0,3)+12) AND $100)<>0,"control_button_64_disabled"}
  !Check{(Peek.w(GTGadPtr(0,4)+14) AND 2)<>0,"control_button_128_immediate"}
  Free GTList 0
!EndTestCase
