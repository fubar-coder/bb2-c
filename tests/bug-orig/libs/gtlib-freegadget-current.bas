; gtlib-freegadget-current (#962) - a GTList keeps its last created
; gadget (+8), which the next creator passes to CreateGadgetA as the
; previous gadget.  GTFreeGadget unlinks and frees the run of gadgets
; with the id, but does not update it: after freeing the last gadget the
; next one is linked behind the freed one (a write into freed memory) and
; is not in the GTList's chain.
; On the Workbench screen two GTButtons (ids 1, 2), GTFreeGadget of id 2,
; a third GTButton (id 3); the chain from the context is searched for it
; (GTGadPtr) and compared with the GTList's last gadget.
; Correct: gadget 3 is in the chain and is the last gadget.
; @fixed #962
; @configs clib
; @pass timeout=60
DEFTYPE .l
WbToScreen 0
GTButton 0,1,10,10,80,14,"one",0
GTButton 0,2,10,30,80,14,"two",0
n=GTFreeGadget(0,2)
GTButton 0,3,10,50,80,14,"three",0
g=GTGadPtr(0,3)
NPrint "freed ",n
NPrint "in chain ",g<>0
NPrint "last ",g=Peek.l(Addr GTList(0)+8)
NPrint "behind 1 ",Peek.l(GTGadPtr(0,1))=g
Free GTList 0
End
