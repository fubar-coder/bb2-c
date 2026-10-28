; wbstartuplib-clistring-ueberlauf (#308) - started from the CLI, the
; argument line is copied into cliString (256 bytes) without a length
; check: a longer line overwrites what is behind it (the libraries linked
; after wbstartuplib) and the program crashes at its start.
; The corpus runner starts programs without arguments, so the program
; runs itself as a child (dos GetProgramName and Execute, as in the corpus
; test t41) with an argument of 1006 characters; the child shows it.
; Correct: the child gets the whole argument and ends normally.
; @configs clib
; @pass timeout=60
DEFTYPE .l
n=NumPars
If n>0
  NPrint "child ",n," ",Len(Par$(1))," ",Left$(Par$(1),3)," ",Right$(Par$(1),3)
  NPrint "child end"
  End
EndIf
; parent: own name (GetProgramName), then Execute "NAME abcxx...xyz"
dn$="dos.library"+Chr$(0)
GetReg d0,&dn$
MOVE.l d0,a1:MOVE.l 4,a6:JSR -408(a6)
PutReg d0,dosb
bk$=String$(Chr$(0),32) : bk=&bk$ : Poke.l bk,dosb
nm$=String$(Chr$(0),256) : Poke.l bk+4,&nm$
GetReg d0,bk
MOVE.l d0,a0:MOVE.l (a0),a6:MOVE.l 4(a0),d1:MOVE.l #255,d2:JSR -576(a6)
c$=Peek$(&nm$)+" abc"+String$("x",1000)+"xyz"+Chr$(0) : Poke.l bk+8,&c$
NPrint "parent"
GetReg d0,bk
MOVEM.l d2-d7/a2-a6,-(a7)
MOVE.l d0,a2:MOVE.l (a2),a6:JSR -60(a6):MOVE.l d0,d3:MOVE.l 8(a2),d1:MOVEQ #0,d2:JSR -222(a6)
MOVEM.l (a7)+,d2-d7/a2-a6
PutReg d0,rc
NPrint "execute ",rc
End
