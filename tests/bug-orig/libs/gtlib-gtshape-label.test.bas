; gtlib-gtshape-label (#172) - GTShape does not set the label
; (ng_GadgetText) of the static NewGadget: it keeps that of the gadget
; made before, and GadTools gets that text for the shape gadget (with the
; GadTools of Kickstart 2 it is the gadget's GadgetText; the button image
; of Kickstart 3.1, which holds the label, is replaced by the shape).
; Here the program patches gadtools.library itself (SetFunction): its
; CreateGadgetA (LVO -30) records ng_GadgetText (+8 of the NewGadget in
; a1) and goes on to the original; the vector is restored afterwards.
; On the Workbench screen a GTButton "Ende", then a GTShape.
; Correct: the button gets a label, the shape gadget none (0).
DEFTYPE .l

!TestCase{shape_label}
  WbToScreen 0
  InitShape 0,16,8,2
  lb=OpenLibrary_("gadtools.library",37)
  !Require{lb<>0,"gadtools_open"}
  code=AllocMem_(32,$10001)
  Poke.w code,$23E9 : Poke.w code+2,8 : Poke.l code+4,code+20  ; move.l 8(a1),data
  Poke.w code+8,$2F39 : Poke.l code+10,code+24                 ; move.l old,-(sp)
  Poke.w code+14,$4E75                                         ; rts
  Poke.l code+20,-1 : Poke.l code+24,0                         ; data, old
  Forbid_
  o=SetFunction_(lb,-30,code)
  Poke.l code+24,o
  Permit_
  GTButton 0,1,10,10,80,14,"Ende",0
  t1=Peek.l(code+20)
  Poke.l code+20,-1
  GTShape 0,2,10,30,0,0
  t2=Peek.l(code+20)
  Forbid_
  dummy=SetFunction_(lb,-30,o)
  Permit_
  !Check{GTGadPtr(0,1)<>0,"button_created"}
  !Check{GTGadPtr(0,2)<>0,"shape_created"}
  !Check{t1<>0 AND t1<>-1,"button_label"}
  !CheckEq{t2,0,"shape_label_none"}
  !Check{t2<>t1,"shape_label_differs"}
  Free GTList 0
  FreeMem_ code,32
  CloseLibrary_ lb
!EndTestCase
