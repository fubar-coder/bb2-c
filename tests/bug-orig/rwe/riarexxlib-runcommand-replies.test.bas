; riarexxlib-runcommand-replies (#531) - a reply to a message sent with
; RXRunCommand is looked up in the sent list by _finditem, which always
; returns 0: the reply is only freed with FreeVec, its argument strings
; stay allocated and the message stays in the sent list, which grows by
; one per message and from the 33rd on is written behind its 256-byte
; block.  The program runs through inject (runwrap=, inject track: what
; the process leaves allocated; guard: writes behind a block) and sends
; 40 two-word commands to its own port, each received, replied and its
; reply read; then it deletes the port.
; Correct: every command is sent, nothing is left allocated, nothing is
; written behind a block.
DEFTYPE .l

!TestCase{forty_commands}
  p=RXCreatePort("BUGORIG")
  RXTokenise On
  s=0
  For i=1 To 40
    r=RXRunCommand("BUGORIG","cmd "+Str$(i))
    If r=-1 Then s+1
    m=RXReadPort
    If m Then RXReplyMsg 0
    m=RXReadPort
  Next
  !CheckEq{s,40,"sent"}
  RXDeletePort
!EndTestCase
