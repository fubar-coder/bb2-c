; rundebuglib-ctrlc-edit (#43) - as rundebuglib-ctrlc-console, for the
; input: an executable compiled with runtime error checking cannot be
; stopped with Ctrl-C.  The program runs itself (dos Execute) as a child
; with an argument and the input from a file of one line, written
; first (RAM:ctrlc-edit, as editlib-edit-eof does with NIL:); the
; child sends itself Ctrl-C (exec SetSignal of SIGBREAKF_CTRL_C) and
; reads a line with Edit$ and a number with Edit from the default
; input (the file), clears the signal after each and prints "not
; stopped".
; Correct (as a C program that reads its standard input): Edit$ ends
; the child with "***Break"; the second child, for Edit, too.
; @configs clib
; @pass runtime_errors=on
; @pass timeout=60
dosb.l=0 : args.l=0
dn$="dos.library"+Chr$(0)
GetReg d0,&dn$
MOVE.l d0,a1:MOVE.l 4,a6:JSR -408(a6)
PutReg d0,dosb.l
bk$=String$(Chr$(0),32) : bk.l=&bk$ : Poke.l bk,dosb
; GetArgStr: "s" (Edit$) or "f" (Edit) in the child
GetReg d0,bk
MOVE.l d0,a0:MOVE.l (a0),a6:JSR -534(a6)
PutReg d0,args.l
If Peek.b(args)=Asc("s")
  SetSignal_ $1000,$1000
  a$=Edit$(10)
  SetSignal_ 0,$1000
  NPrint "edit$ not stopped"
  End
EndIf
If Peek.b(args)=Asc("f")
  SetSignal_ $1000,$1000
  f.f=Edit(5)
  SetSignal_ 0,$1000
  NPrint "edit not stopped"
  End
EndIf
If WriteFile(0,"RAM:ctrlc-edit")
  FileOutput 0 : NPrint "12" : CloseFile 0
EndIf
DefaultOutput
nm$=String$(Chr$(0),256) : Poke.l bk+4,&nm$
GetReg d0,bk
MOVE.l d0,a0:MOVE.l (a0),a6:MOVE.l 4(a0),d1:MOVE.l #255,d2:JSR -576(a6)
For i=1 To 2
  If i=1 Then c$=Peek$(&nm$)+" s <RAM:ctrlc-edit"+Chr$(0) Else c$=Peek$(&nm$)+" f <RAM:ctrlc-edit"+Chr$(0)
  Poke.l bk+8,&c$
  NPrint "child ",i
  GetReg d0,bk
  MOVEM.l d2-d7/a2-a6,-(a7)
  MOVE.l d0,a2:MOVE.l (a2),a6:JSR -60(a6):MOVE.l d0,d3:MOVE.l 8(a2),d1:MOVEQ #0,d2:JSR -222(a6)
  MOVEM.l (a7)+,d2-d7/a2-a6
Next
NPrint "end"
End
