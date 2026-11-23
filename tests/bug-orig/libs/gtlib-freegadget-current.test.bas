; gtlib-freegadget-current (#962) - a GTList keeps its last created
; gadget (+8), which the next creator passes to CreateGadgetA as the
; previous gadget.  GTFreeGadget unlinks and frees the run of gadgets
; with the id, but does not update it: after freeing the last gadget the
; next one is linked behind the freed one (a write into freed memory) and
; is not in the GTList's chain.
; Here: on the Workbench screen two GTButtons (ids 1, 2), GTFreeGadget of
; id 2, a third GTButton (id 3); the chain from the context is searched
; for it (GTGadPtr) and compared with the GTList's last gadget.
; Correct: gadget 3 is in the chain and is the last gadget.
DEFTYPE .l

!TestCase{free_last_gadget}
  WbToScreen 0
  GTButton 0,1,10,10,80,14,"one",0
  GTButton 0,2,10,30,80,14,"two",0
  n=GTFreeGadget(0,2)
  GTButton 0,3,10,50,80,14,"three",0
  g=GTGadPtr(0,3)
  !CheckEq{n,1,"freed"}
  !Check{g<>0,"in_chain"}
  !Check{g=Peek.l(Addr GTList(0)+8),"is_last"}
  !Check{Peek.l(GTGadPtr(0,1))=g,"behind_gadget_1"}
  Free GTList 0
!EndTestCase
