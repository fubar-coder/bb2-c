; rimapeditorlib-candraw-no-stencil (#893) - MapCanDraw looks up the map
; word of the position in the stencil of SetStencil without looking
; whether there is one: before the first SetStencil the stencil is at
; address 0, so MapCanDraw reads the byte at the address "map word" in
; low memory (here the map word 0: the byte at address 0), and its
; result depends on that memory.  A command before its setup command;
; the original has no error check area.  Here with runtime error
; checking.
; Correct: the error check refuses MapCanDraw without a SetStencil:
; "MapCanDraw needs SetStencil".
; @rwe rimapeditorlib
; @pass runtime_errors=on
CreateMap 0,4,4,16,16,"m"
NPrint "before"
r=MapCanDraw(3,1,1)
NPrint "after"
End
