; t101_brexx - brexxlib: events go to a tape (TapeTrap Tape#, QuietTrap
; On: nothing is executed): Type, AbsMouse, RelMouse, MouseButton,
; ClickButton (BUG(orig) (#97): button up with the QuietTrap value), XStatus,
; SaveTape/LoadTape round trip, MacroKey/FreeMacroKey, Record/PlayBack
; off, QuickPlay.  No event reaches the input stream.
DEFTYPE .l
Function.l cks{a.l,n.l}
  c.l=0
  For i.l=0 To n-1
    c=(c*3+(Peek.b(a+i) AND 255)) AND $FFFFFF
  Next
  Function Return c
End Function
Statement fdump{f$}
  If ReadFile(0,f$)
    l.l=Lof(0)
    m.l=AllocMem(l+8,65536)
    ReadMem 0,m,l
    CloseFile 0
    NPrint f$," len ",l," cks ",Hex$(cks{m,l})
    s$=""
    For i.l=0 To l-1
      s$=s$+Right$(Hex$(Peek.b(m+i) AND 255),2)
      If i=7 OR ((i-8) MOD 18)=17 Then NPrint "  ",s$ : s$=""
    Next
    If s$<>"" Then NPrint "  ",s$
    FreeMem m,l+8
  Else
    NPrint f$," missing"
  EndIf
End Statement
Statement showtape{n.l}
  a.l=Addr Tape(n)
  c.l=0 : e.l=Peek.l(a)
  While e<>0
    c=c+1 : e=Peek.l(e)
  Wend
  NPrint "tape ",n," events ",c," time ",Peek.l(a+4)," ",Peek.l(a+8)
End Statement
NPrint "status ",XStatus
Record : PlayBack
NPrint "off ",XStatus
QuietTrap 1
TapeTrap 0
Type "Ab1 ."
NPrint "type ",XStatus
AbsMouse 10,20
RelMouse -3,4
MouseButton 0,1
MouseButton 1,0
ClickButton 0
ClickButton 1
NPrint "mouse ",XStatus
showtape{0}
SaveTape 0,"RAM:t101a.tape"
fdump{"RAM:t101a.tape"}
LoadTape 1,"RAM:t101a.tape"
showtape{1}
SaveTape 1,"RAM:t101b.tape"
fdump{"RAM:t101b.tape"}
SaveTape 2,"RAM:t101c.tape"
NPrint "empty tape saved ",Exists("RAM:t101c.tape")
LoadTape 1,"RAM:t101b.tape"
showtape{1}
QuickPlay 1 : QuickPlay 0
MacroKey 1,$45,0
MacroKey 1,$45,0
MacroKey 0,$46,8
FreeMacroKey $45,0
FreeMacroKey $46,8
FreeMacroKey $47,0
NPrint "macros ",XStatus
TapeTrap 2
Type "x"
showtape{2}
NPrint "end ",XStatus
KillFile "RAM:t101a.tape" : KillFile "RAM:t101b.tape"
End
