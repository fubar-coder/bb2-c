; riarexxlib-executescript (#544) - RXExecuteScript and RXRunScript set
; their message type, but _rxruncommand overwrites it: both send
; RXCOMM|RXFF_STRING, the script name as a tokenised string program, so
; no script is loaded, and neither waits.  The program starts RexxMast
; (not started in the sessions; SYS:System/RexxMast writes nothing),
; writes three scripts to RAM: and runs them: one that ends with code 0,
; one with a syntax error (RXExecuteScript; RXReturnCode right after it),
; and one that sends the command "hello" to the program's port
; (RXRunScript; the program then waits for that command and replies it).
; Correct (user decision W4 Q1 (b)): RXExecuteScript waits for the end of
; the script and returns -1, RXReturnCode its code (0, then 10);
; RXRunScript returns -1 and the script's "hello" arrives.
; @fixed #544
; @rwe riarexxlib
DEFTYPE .l
r$="REXX"
If FindPort_(&r$)=0
  c$="SYS:System/RexxMast >NIL:"
  Execute_ &c$,0,0
EndIf
w=0
While FindPort_(&r$)=0 AND w<100
  Delay_ 5 : w+1
Wend
x=FindPort_(&r$)
NPrint "rexx ",x<>0
q$=Chr$(39)
If WriteFile(0,"RAM:bugorig-ok.rexx") Then FileOutput 0 : NPrint "/* */" : NPrint "exit 0" : CloseFile 0
If WriteFile(0,"RAM:bugorig-bad.rexx") Then FileOutput 0 : NPrint "/* */" : NPrint "x = (" : CloseFile 0
t$="ADDRESS BUGORIG "+q$+"hello"+q$
If WriteFile(0,"RAM:bugorig-run.rexx") Then FileOutput 0 : NPrint "/* */" : NPrint t$ : CloseFile 0
DefaultOutput
p=RXCreatePort("BUGORIG")
RXTokenise On
r=RXExecuteScript("RAM:bugorig-ok.rexx")
NPrint "ok: ",r=-1," rc ",RXReturnCode
r=RXExecuteScript("RAM:bugorig-bad.rexx")
NPrint "bad: ",r=-1," rc ",RXReturnCode
r=RXRunScript("RAM:bugorig-run.rexx")
NPrint "run: ",r=-1
m=0 : w=0
While m=0 AND w<50
  m=RXReadPort
  If m=0 Then Delay_ 5 : w+1
Wend
If m
  NPrint "got <",RXGetArg(1),">"
  RXReplyMsg 0
Else
  NPrint "nothing arrived"
EndIf
Delay_ 25
m=RXReadPort
KillFile "RAM:bugorig-ok.rexx"
KillFile "RAM:bugorig-bad.rexx"
KillFile "RAM:bugorig-run.rexx"
End
