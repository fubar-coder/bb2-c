; gadgetslib-selectmode-noop (#952) - SelectMode (manual: "1 for box and 0 for
; inverse. Use prior to creation of gadgets") only stores the mode in
; selmode, which nothing reads: every gadget gets defflags (never set,
; 0 = GADGHCOMP) as its Flags.  The first gadget of the list (the
; GadgetList object +0), Flags at +12, highlight bits 0-1.
; Correct: after SelectMode 1 the gadget has GADGHBOX (1).
; @configs clib
DEFTYPE .l
SelectMode 1
TextGadget 0,10,10,0,1,"Box"
g=Peek.l(Addr GadgetList(0))
NPrint "highlight ",Peek.w(g+12) AND 3
End
