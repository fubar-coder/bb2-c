; gadgetslib-selectmode-noop (#952) - SelectMode (manual: "1 for box and 0 for
; inverse. Use prior to creation of gadgets") only stores the mode in
; selmode, which nothing reads: every gadget gets defflags (never set,
; 0 = GADGHCOMP) as its Flags.  The first gadget of the list (the
; GadgetList object +0), Flags at +12, highlight bits 0-1.
; Correct: after SelectMode 1 the gadget has GADGHBOX (1); a shape gadget
; with an alternative shape keeps GADGHIMAGE (2), and after SelectMode 0
; a gadget has GADGHCOMP (0) again.
; @fixed #952
; @configs clib
DEFTYPE .l
InitShape 0,16,8,1
InitShape 1,16,8,1
SelectMode 1
TextGadget 0,10,10,0,1,"Box"
ShapeGadget 0,40,10,0,2,0,1
SelectMode 0
TextGadget 0,80,10,0,3,"Comp"
g=Peek.l(Addr GadgetList(0))
NPrint "highlight ",Peek.w(g+12) AND 3
g=Peek.l(g)
NPrint "shape ",Peek.w(g+12) AND 3
g=Peek.l(g)
NPrint "after 0 ",Peek.w(g+12) AND 3
End
