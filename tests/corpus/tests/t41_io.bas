; t41_io - inputoutputlib: DefaultOutput/PopOutput, DefaultInput/PopInput,
; the channel stacks, and the default input (dos Input()): the program runs
; itself (dos Execute) as a child with an argument and input from a file.
dosb.l=0 : args.l=0 : rc.l=0
dn$="dos.library"+Chr$(0)
GetReg d0,&dn$
MOVE.l d0,a1:MOVE.l 4,a6:JSR -408(a6)
PutReg d0,dosb.l
bk$=String$(Chr$(0),32) : bk.l=&bk$ : Poke.l bk,dosb
; GetArgStr: "x" in the child, an empty line otherwise
GetReg d0,bk
MOVE.l d0,a0:MOVE.l (a0),a6:JSR -534(a6)
PutReg d0,args.l
If Peek.b(args)=Asc("x")
  ; ---------------- child: Edit$/Edit through DefaultInput (dos Read)
  NPrint "child"
  a$=Edit$(5) : NPrint "edit$5 [",a$,"] ",Len(a$)
  a$=Edit$("dflt",3) : NPrint "edit$dflt3 [",a$,"] ",Len(a$)
  g.f=Abs(g) : f.f=Edit(4) : NPrint "edit4 [",f,"]"
  g.f=Abs(g) : f.f=Edit(9.5,5) : NPrint "edit9.5,5 [",f,"]"
  a$=Edit$(10) : NPrint "edit$10 [",a$,"] ",Len(a$)
  a$=Edit$(2) : NPrint "edit$2 [",a$,"] ",Len(a$)
  a$=Edit$(4) : NPrint "edit$4 [",a$,"] ",Len(a$)
  DefaultInput
  a$=Edit$(3) : NPrint "default [",a$,"]"
  PopInput
  a$=Edit$(1) : NPrint "pop [",a$,"]"
  a$=Edit$("abcdefgh",3) : NPrint "edit$long [",a$,"] ",Len(a$)
  NPrint "child end"
  End
EndIf
NPrint "parent"
; guard: a child that does not see its argument must not run itself again
k$="RAM:bb2t41.lock"
If Exists(k$) Then NPrint "nested or stale lock" : End
; ---------------- output stack with a file
f$="RAM:bb2t41.out"
If WriteFile(0,f$)
  FileOutput 0
  NPrint "line1 to file"
  DefaultOutput
  NPrint "to stdout 1"
  PopOutput
  Print "line2 ",42," ",-1.5
  NPrint ""
  DefaultOutput
  DefaultOutput
  NPrint "to stdout 2"
  PopOutput
  NPrint "to stdout 3"
  PopOutput
  NPrint "line3 to file"
  PopOutput
  NPrint "to stdout 4"
  CloseFile 0
EndIf
If ReadFile(0,f$)
  FileInput 0
  a$=Edit$(80) : NPrint "read1 [",a$,"]"
  DefaultInput
  PopInput
  a$=Edit$(80) : NPrint "read2 [",a$,"]"
  PopInput
  FileInput 0
  a$=Edit$(80) : NPrint "read3 [",a$,"]"
  a$=Edit$(80) : NPrint "read4 [",a$,"] ",Len(a$)
  PopInput
  CloseFile 0
EndIf
; ring of 16 entries: 20 pushes, 20 pops
For i=1 To 20 : DefaultOutput : Next
For i=1 To 20 : PopOutput : Next
NPrint "after ring"
; ---------------- input file for the child, then run it
i$="RAM:bb2t41.in"
If WriteFile(1,i$)
  FileOutput 1
  NPrint "abcde"
  NPrint "xyz"
  NPrint "12.5"
  NPrint "-3.75"
  NPrint "0123456789"
  NPrint "toolong"
  NPrint "end"
  NPrint "Q"
  NPrint "uvw"
  NPrint "trailing data"
  PopOutput
  CloseFile 1
EndIf
If WriteFile(2,k$) Then CloseFile 2
nm$=String$(Chr$(0),256) : Poke.l bk+4,&nm$
GetReg d0,bk
MOVE.l d0,a0:MOVE.l (a0),a6:MOVE.l 4(a0),d1:MOVE.l #255,d2:JSR -576(a6)
c$=Peek$(&nm$)+" x <"+i$+Chr$(0) : Poke.l bk+8,&c$
GetReg d0,bk
MOVEM.l d2-d7/a2-a6,-(a7)
MOVE.l d0,a2:MOVE.l (a2),a6:JSR -60(a6):MOVE.l d0,d3:MOVE.l 8(a2),d1:MOVEQ #0,d2:JSR -222(a6)
MOVEM.l (a7)+,d2-d7/a2-a6
PutReg d0,rc.l
NPrint "child rc ",rc
KillFile f$ : KillFile i$ : KillFile k$
NPrint "end"
End
