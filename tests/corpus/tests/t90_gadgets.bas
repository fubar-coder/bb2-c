; t90_gadgets - gadgetslib: a gadget list (text, cycling text, string,
; prop, shape gadgets) dumped field by field, the state commands, then
; the list in a window (Redraw, ActivateString, GadgetBorder); no waiting
WbToScreen 0
gl.l=Addr GadgetList(0)
GadgetPens 2,3 : BorderPens 3,4 : GadgetJam 2 : ButtonGroup 2 : SelectMode 1
TextGadget 0,10,20,$201,1,"Hello|World|X"
TextGadget 0,8,7,$1F03E,2,"Abc"
StringGadget 0,30,40,$20,5,20,100
PropGadget 0,1,2,$1CF,6,50,12
PropGadget 0,3,4,$40,9,40,10
Borders 0
TextGadget 0,1,2,$200,3,"one|two|three"
Borders 1
Borders 6,3
GadgetPens 1
TextGadget 0,40,50,0,10,"Six"
InitShape 0,16,8,2
InitShape 1,12,6,1
ShapeGadget 0,50,60,1,7,0
ShapeGadget 0,50,60,2,8,0,1
g.l=Peek.l(gl)
While g
  Print "id ",Peek.w(g+38)," pos ",Peek.w(g+4),",",Peek.w(g+6)
  Print " size ",Peek.w(g+8),",",Peek.w(g+10)," fl ",Hex$(Peek.l(g+12))
  Print " ty ",Hex$(Peek.w(g+16) AND $FFFF)," ud ",Hex$(Peek.l(g+40))
  NPrint " r ",Peek.l(g+18)<>0," s ",Peek.l(g+22)<>0
  it.l=Peek.l(g+26)
  If it
    Print "  text [",Peek$(Peek.l(it+12)),"] [",Peek$(Peek.l(it+20)),"] "
    NPrint Peek.b(it)," ",Peek.b(it+1)," ",Peek.b(it+2)," ",Peek.w(it+4)," ",Peek.w(it+6)
  EndIf
  b.l=Peek.l(g+18)
  ; border: LeftEdge, TopEdge, pens, mode, count and the points (not the
  ; XY/NextBorder pointers, they depend on the memory layout)
  If b<>0 AND (Peek.w(g+12) AND 4)=0
    s$="  border"
    For i=0 To 7 Step 2 : s$=s$+" "+Str$(Peek.w(b+i)) : Next
    For i=16 To 35 Step 2 : s$=s$+" "+Str$(Peek.w(b+i)) : Next
    NPrint s$
    s$="  shadow"
    For i=36 To 43 Step 2 : s$=s$+" "+Str$(Peek.w(b+i)) : Next
    For i=52 To 71 Step 2 : s$=s$+" "+Str$(Peek.w(b+i)) : Next
    NPrint s$
  EndIf
  If b<>0 AND (Peek.w(g+12) AND 4)<>0
    NPrint "  image ",Peek.w(b+4)," ",Peek.w(b+6)," ",Peek.w(b+8)," ",Peek.b(b+14) AND 255
  EndIf
  si.l=Peek.l(g+34)
  If si<>0 AND (Peek.w(g+16) AND 15)=4 Then NPrint "  string max ",Peek.w(si+10)," buf ",Peek.b(Peek.l(si))
  If si<>0 AND (Peek.w(g+16) AND 15)=3
    NPrint "  prop ",Hex$(Peek.w(si) AND $FFFF)," ",Hex$(Peek.l(si+2))," ",Hex$(Peek.l(si+6))
  EndIf
  g=Peek.l(g)
Wend
SetHProp 0,6,1,0.5
SetVProp 0,6,0.25,0.125
NPrint "prop ",HPropPot(0,6)," ",HPropBody(0,6)," ",VPropPot(0,6)," ",VPropBody(0,6)
Toggle 0,1,1
Toggle 0,2
NPrint "status ",GadgetStatus(0,1)," ",GadgetStatus(0,2)," ",GadgetStatus(0,3)," ",GadgetStatus(0,5)
SetGadgetStatus 0,3,3
NPrint "cycle 3 ",GadgetStatus(0,3)
SetGadgetStatus 0,3,2
NPrint "cycle 2 ",GadgetStatus(0,3)
NPrint "button ",ButtonId(0,2)," ",ButtonId(0,7)
Disable 0,5 : Enable 0,1
SetString 0,5,"some text"
NPrint "string [",StringText$(0,5),"]"
ResetString 0,5
ClearString 0,5
NPrint "cleared [",StringText$(0,5),"]"
SetTextGadget 0,10,"Hi"
SetTextGadget 0,2,"longer than this"
g=Peek.l(gl)
While g
  it.l=Peek.l(g+26)
  If it Then NPrint "text ",Peek.w(g+38)," [",Peek$(Peek.l(it+12)),"]"
  g=Peek.l(g)
Wend
; a second, simple list for the window (list 0 has border/GZZ gadgets)
TextGadget 1,10,20,0,30,"Win"
StringGadget 1,10,40,0,31,20,100
Window 0,10,10,320,160,$100E,"t90",1,2,1
DefaultOutput
If Peek.l(Addr Window(0))=0
  NPrint "no window"
  End
EndIf
NPrint "window ",Peek.w(Peek.l(Addr Window(0))+8)," ",Peek.l(Peek.l(Addr Window(0))+62)=Peek.l(Addr GadgetList(1))
Redraw 0,30
ActivateString 0,31
GadgetBorder 10,10,40,20
NPrint "window ok"
CloseWindow 0
NPrint "done"
End
