; editlib-edit-eof (#132) - at the end of the file the default input
; (DefaultInput: dos Read(Input(),buffer,n+1)-1) returns -1 characters;
; Edit$ adds that to a3 unchecked and returns the length -1 (a3 one byte
; back), Edit gives Val the length -1.  The program runs itself (dos
; Execute) as a child with an argument and the input from NIL: (as the
; corpus test t41 does with a file).
; Correct: at the end of the file Edit$ is the empty string and Edit 0.
; @fixed #132
; @configs clib
; @pass timeout=60
dosb.l=0 : args.l=0 : rc.l=0
dn$="dos.library"+Chr$(0)
GetReg d0,&dn$
MOVE.l d0,a1:MOVE.l 4,a6:JSR -408(a6)
PutReg d0,dosb.l
bk$=String$(Chr$(0),32) : bk.l=&bk$ : Poke.l bk,dosb
; GetArgStr: "x" in the child
GetReg d0,bk
MOVE.l d0,a0:MOVE.l (a0),a6:JSR -534(a6)
PutReg d0,args.l
If Peek.b(args)=Asc("x")
  NPrint "edit$ ",Len(Edit$(10))
  NPrint "edit$dflt ",Len(Edit$("ab",5))
  g.f=Abs(g) : f.f=Edit(5) : NPrint "edit ",f
  End
EndIf
nm$=String$(Chr$(0),256) : Poke.l bk+4,&nm$
GetReg d0,bk
MOVE.l d0,a0:MOVE.l (a0),a6:MOVE.l 4(a0),d1:MOVE.l #255,d2:JSR -576(a6)
c$=Peek$(&nm$)+" x <NIL:"+Chr$(0) : Poke.l bk+8,&c$
GetReg d0,bk
MOVEM.l d2-d7/a2-a6,-(a7)
MOVE.l d0,a2:MOVE.l (a2),a6:JSR -60(a6):MOVE.l d0,d3:MOVE.l 8(a2),d1:MOVEQ #0,d2:JSR -222(a6)
MOVEM.l (a7)+,d2-d7/a2-a6
NPrint "end"
End
